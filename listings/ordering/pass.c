bool
symbol_topsort(darr_T(C_Symbol) initials, C_SymbolTable table, darr_T(C_Symbol) *out_sorted, Allocator *alloc) {
    darr_t sorted;
    ASSERT_OK(darr_new_cap_in_T(C_Symbol, hashmap_len(table), alloc, &sorted));

    darr_t stack = initials;

    while (darr_len(stack) > 0) {
        C_Symbol cur = *darr_get_iT(C_Symbol, stack, -1);
        darr_pop(&stack);
        usize_t *cur_in_deg = &hashmap_get_T(C_SymbolData, table, &cur)->in_deg;
        ASSERT(*cur_in_deg == 0);

        hashset_T(C_Symbol) f_deps = hashmap_get_T(C_SymbolData, table, &cur)->f_deps;
        for_in_range(i, 0ul, hashset_len(f_deps)) {
            C_Symbol f_dep = *slice_get_T(C_Symbol, &hashset_values_raw(f_deps), i);
            usize_t *f_dep_in_deg = &hashmap_get_T(C_SymbolData, table, &f_dep)->in_deg;
            ASSERT(*f_dep_in_deg > 0);
            *f_dep_in_deg -= 1;
            if (*f_dep_in_deg == 0) {
                ASSERT_OK(darr_push(&stack, &f_dep));
            }
        }

        ASSERT_OK(darr_push(&sorted, &cur));
    }

    if (darr_len(sorted) != hashmap_len(table)) {
        return false;
    }

    *out_sorted = sorted;

    return true;
}


bool
init_owner(str_t owner_name, C_Ast_Node *node, C_SymbolTable *scope, C_SymbolTable *f_scope, Allocator *alloc) {
    // deps
    auto data = hashmap_get_T(C_SymbolData, *scope, &owner_name);
    if (data != nullptr) {
        eprint_fmt(S("redefinition of %s\n"), owner_name);
        return false;
    } 
    hashset_T(C_Symbol) deps;
    ASSERT_OK(hashset_new_cap_in_T(C_Symbol, 32, alloc, &deps));
    ASSERT_OK(hashmap_set(scope, &owner_name, &(C_SymbolData) {
        .node = node,
        .deps = deps,
    }));
    data = hashmap_get_T(C_SymbolData, *scope, &owner_name);

    // f_deps
    data = hashmap_get_T(C_SymbolData, *f_scope, &owner_name);
    if (data != nullptr) {
        // allow other to add forward dependencies
        return true;
    } 

    hashset_T(C_Symbol) f_deps;
    ASSERT_OK(hashset_new_cap_in_T(C_Symbol, 32, alloc, &f_deps));
    ASSERT_OK(hashmap_set(f_scope, &owner_name, &(C_SymbolData) {
        .f_deps = f_deps,
    }));
    data = hashmap_get_T(C_SymbolData, *f_scope, &owner_name);

    return true;
}

void
ec_collect_symbol_deps_decl(C_Ast_Decl *decl, hashset_T(C_Symbol) *deps, C_SymbolTable *f_scope, C_Symbol owner_name);

/// @brief Assumes there are no proc_macro directives left
/// @param self 
/// @return true on success
bool
ec_translation_unit_order(C_TranslationUnitData *self) {
    Arena order_arena;
    ASSERT_OK(arena_init(&order_arena, 4096, ctx_global_alloc));
    auto order_alloc = arena_allocator(&order_arena);

    C_SymbolTable deps_table;
    C_SymbolTable f_deps_table;
    ASSERT_OK(hashmap_new_cap_in_T(C_Symbol, C_SymbolData, 32, &order_alloc, &deps_table));
    ASSERT_OK(hashmap_new_cap_in_T(C_Symbol, C_SymbolData, 32, &order_alloc, &f_deps_table));

    darr_T(C_Ast_Decl *) vars; 
    darr_T(C_Ast_Decl *) fn_defs; 
    ASSERT_OK(darr_new_cap_in_T(C_Ast_Decl *, 32, &order_alloc, &vars));
    ASSERT_OK(darr_new_cap_in_T(C_Ast_Decl *, 32, &order_alloc, &fn_defs));

    auto tr_unit = (EC_Ast_TranslationUnit *)self->tr_unit;
    if (darr_len(tr_unit->items) == 0) {
        return true;
    }

    auto ast_alloc = arena_allocator(&self->ast_arena);
    darr_t processed = nullptr;
    ASSERT_OK(darr_new_cap_in_T(EC_Ast_TrUnitItem *, darr_len(tr_unit->items), &ast_alloc, &processed));

    // collect dependencies
    for_in_range(i, 0, darr_len(tr_unit->items)) {
        auto item = *darr_get_T(EC_Ast_TrUnitItem *, tr_unit->items, i);
        if (item->kind == C_AST_NODE_KIND_AT_DIRECTIVE) {
            ASSERT_OK(darr_push(&processed, &item));
            continue;
        }
        ASSERT(item->kind == C_AST_NODE_KIND_DECL);

        switch (item->decl.decl_kind) {
        case C_AST_DECL_KIND_FN_DEF: {
            ASSERT_OK(darr_push(&fn_defs, &item));
            continue;
            break;
        }
        case C_AST_DECL_KIND_TYPEDEF: {
            C_Ast_DeclTypedef *d = &item->decl.d_typedef;
            str_t name = d->name->name;

            if (!init_owner(name, (C_Ast_Node *)item, &deps_table, &f_deps_table, &order_alloc)) {
                return false;
            }
            auto data = hashmap_get_T(C_SymbolData, deps_table, &name);
            ec_collect_symbol_deps_decl((C_Ast_Decl *)item, &data->deps, &f_deps_table, name);
            break;
        }
        case C_AST_DECL_KIND_TYPE_DECL: {
            C_Ast_Type *ty = item->decl.d_type.ty;
            if (ty->ty_kind == C_AST_TYPE_KIND_STRUCT) {
                if (ty->ty_struct.fields == nullptr) {
                    continue;
                }
            } else {
                unimplemented();
            }
            str_t name = ty->ty_struct.name->name;

            if (!init_owner(name, (C_Ast_Node *)item, &deps_table, &f_deps_table, &order_alloc)) {
                return false;
            }
            auto data = hashmap_get_T(C_SymbolData, deps_table, &name);
            ec_collect_symbol_deps_decl((C_Ast_Decl *)item, &data->deps, &f_deps_table, name);
            break;
        }
        case C_AST_DECL_KIND_VARIABLE: {
            ASSERT_OK(darr_push(&vars, &item));
            continue;
            break;
        }
        case C_AST_DECL_KIND_EMPTY: {
            continue;
            break;
        }
        default:
            unreachable();
            break;
        }

    }

    darr_T(C_Symbol) initials;
    ASSERT_OK(darr_new_cap_in_T(C_Symbol, hashmap_len(deps_table), &order_alloc, &initials));
    for_in_range(i, 0, hashmap_len(deps_table)) {
        C_Symbol *key = slice_get_T(C_Symbol, &deps_table->keys, i);
        C_SymbolData *val = slice_get_T(C_SymbolData, &deps_table->values, i);

        val->f_deps = hashmap_get_T(C_SymbolData, f_deps_table, key)->f_deps;
        val->in_deg = hashset_len(val->deps);
        if (val->in_deg == 0) {
            ASSERT_OK(darr_push(&initials, key));
        }
    }

    darr_T(C_Symbol) sorted;
    if (!symbol_topsort(initials, deps_table, &sorted, &order_alloc)) {
        eprint_fmt(S("dependency cycle detected\n"));
        return false;
    }

    // pre-declarations
    for_in_range(i, 0, darr_len(sorted)) {
        C_Symbol sym = *darr_get_T(C_Symbol, sorted, i);
        C_Ast_Node *node = hashmap_get_T(C_SymbolData, deps_table, &sym)->node;
        ASSERT(node->kind = C_AST_NODE_KIND_DECL);
        if (node->decl.decl_kind != C_AST_DECL_KIND_TYPE_DECL) {
            continue;
        }
        C_Ast_DeclType *decl_ty = (C_Ast_DeclType *)node;

        if (decl_ty->ty->ty_kind != C_AST_TYPE_KIND_STRUCT) {
            unimplemented();
        }

        C_Ast_DeclType *predecl = nullptr;
        C_Ast_TypeStruct *predecl_ty = nullptr;

        ASSERT_OK(allocator_alloc_T(&ast_alloc, C_Ast_DeclType, &predecl));
        ASSERT_OK(allocator_alloc_T(&ast_alloc, C_Ast_TypeStruct, &predecl_ty));

        *predecl_ty = (C_Ast_TypeStruct) {
            .kind = C_AST_NODE_KIND_TYPE_NAME, 
            .ty_kind = C_AST_TYPE_KIND_STRUCT,
            .name = decl_ty->ty->ty_struct.name,
            };
        *predecl = (C_Ast_DeclType) {
            .kind = C_AST_NODE_KIND_DECL, 
            .decl_kind = C_AST_DECL_KIND_TYPE_DECL,
            .ty = (C_Ast_Type *)predecl_ty,
            };

        ASSERT_OK(darr_push(&processed, &predecl));

        // typedef struct A A;
        C_Ast_DeclTypedef *tydef = nullptr;
        ASSERT_OK(allocator_alloc_T(&ast_alloc, C_Ast_DeclTypedef, &tydef));
        *tydef = (C_Ast_DeclTypedef) {
            .kind = C_AST_NODE_KIND_DECL, 
            .decl_kind = C_AST_DECL_KIND_TYPEDEF,
            .ty = (C_Ast_Type *)predecl_ty,
            .name = decl_ty->ty->ty_struct.name,
        };

        ASSERT_OK(darr_push(&processed, &tydef));
    }

    for_in_range(i, 0, darr_len(sorted)) {
        C_Symbol sym = *darr_get_T(C_Symbol, sorted, i);
        ASSERT_OK(darr_push(&processed, 
            &hashmap_get_T(C_SymbolData, deps_table, &sym)->node));
    }

    for_in_range(i, 0, darr_len(vars)) {
        ASSERT_OK(darr_push(&processed, 
            darr_get(vars, i)));
    }

    // headers
    for_in_range(i, 0, darr_len(fn_defs)) {
        C_Ast_DeclFnDef *fn_def = *darr_get_T(C_Ast_DeclFnDef *, fn_defs, i);
        C_Ast_DeclVar *fn_head = nullptr;

        ASSERT_OK(allocator_alloc_T(&ast_alloc, C_Ast_DeclVar, &fn_head));

        *fn_head = (C_Ast_DeclVar) {
            .kind = C_AST_NODE_KIND_DECL, 
            .decl_kind = C_AST_DECL_KIND_VARIABLE,
            .ty = fn_def->ty,
            .name = fn_def->name,
            };

        ASSERT_OK(darr_push(&processed, &fn_head));
    }

    for_in_range(i, 0, darr_len(fn_defs)) {
        ASSERT_OK(darr_push(&processed, 
            darr_get(fn_defs, i)));
    }


    tr_unit->items = processed;

    arena_deinit(&order_arena);

    return true;
}

bool
str_is_c_primitive_type(str_t name) {
    if (str_eq(name, S("int"))) {
        return true;
    }
    return false;
}

static
void
_record_dep(C_Symbol dep_name, hashset_T(C_Symbol) *deps, C_SymbolTable *f_scope, C_Symbol owner_name) {
    if (str_eq(dep_name, owner_name)) {
        return;
    }
    if (str_is_c_primitive_type(dep_name)) {
        return;
    }

    ASSERT_OK(hashset_add(deps, &dep_name));
    auto data = hashmap_get_T(C_SymbolData, *f_scope, &dep_name);
    if (data == nullptr) {
        hashset_T(C_Symbol) f_deps;
        hashset_new_cap_in_T(C_Symbol, 32, ctx_global_alloc, &f_deps);
        ASSERT_OK(hashmap_set(f_scope, &dep_name, &(C_SymbolData) {
            .f_deps = f_deps,
        }));
        data = hashmap_get_T(C_SymbolData, *f_scope, &dep_name);
    } 

    ASSERT_OK(hashset_add(&data->f_deps, &owner_name));
}


/// @note doesn't record weak(pointer) dependencies
/// @param[in, out] deps deps of the owner
/// @param[in, out] f_scope table of [name, f_deps], to record owner in
/// @param[in] decl_name owner name
void
ec_collect_symbol_deps_ty(C_Ast_Type *ty, hashset_T(C_Symbol) *deps, C_SymbolTable *f_scope, C_Symbol owner_name) {
    switch (ty->ty_kind)
    {
    case C_AST_TYPE_KIND_IDENT: {
        _record_dep(ty->ty_ident.ident->name, deps, f_scope, owner_name);
        break;
    }
    case C_AST_TYPE_KIND_POINTER:
        // record only strong dependencies
        break;
    case C_AST_TYPE_KIND_ARRAY: {
        if (ty->ty_array.count != nullptr) {
            unimplemented();
        }
        
        ec_collect_symbol_deps_ty(ty->ty_array.item, deps, f_scope, owner_name);
        break;
    }
    case C_AST_TYPE_KIND_FUNCTION: {
        if (ty->ty_fn.args) {
            for_in_range(i, 0, darr_len(ty->ty_fn.args)) {
                auto arg = darr_get_T(C_Ast_FnParam, ty->ty_fn.args, i);
                ec_collect_symbol_deps_ty(arg->ty, deps, f_scope, owner_name);
            }
        }
        ec_collect_symbol_deps_ty(ty->ty_fn.ret, deps, f_scope, owner_name);
        break;
    }
    case C_AST_TYPE_KIND_STRUCT: {
        auto rec = (C_Ast_TypeRecord *)&ty->ty_struct;
        _record_dep(rec->name->name, deps, f_scope, owner_name);
        if (rec->fields) {
            for_in_range(i, 0, darr_len(rec->fields)) {
                auto arg = darr_get_T(C_Ast_Decl, rec->fields, i);
                ec_collect_symbol_deps_decl(arg, deps, f_scope, owner_name);
            }
        }
        break;
    }
    case C_AST_TYPE_KIND_UNION:
    case C_AST_TYPE_KIND_ENUM:
        unimplemented();
        break;
    
    default:
        unreacheble();
        break;
    }
}

void
ec_collect_symbol_deps_decl(C_Ast_Decl *decl, hashset_T(C_Symbol) *deps, C_SymbolTable *f_scope, C_Symbol owner_name) {
    switch (decl->decl_kind)
    {
    case C_AST_DECL_KIND_TYPEDEF: {
        ec_collect_symbol_deps_ty(decl->d_typedef.ty, deps, f_scope, owner_name);
        if (decl->d_typedef.others) {
            for_in_range(i, 0, darr_len(decl->d_typedef.others)) {
                auto dec = darr_get_T(C_Ast_Declarator, decl->d_typedef.others, i);
                ec_collect_symbol_deps_ty(dec->ty, deps, f_scope, owner_name);
            }
        }
        break;
    }
    case C_AST_DECL_KIND_TYPE_DECL: {
        ec_collect_symbol_deps_ty(decl->d_type.ty, deps, f_scope, owner_name);
        break;
    }
    case C_AST_DECL_KIND_VARIABLE: {
        ec_collect_symbol_deps_ty(decl->d_var.ty, deps, f_scope, owner_name);
        if (decl->d_var.others) {
            for_in_range(i, 0, darr_len(decl->d_var.others)) {
                auto dec = darr_get_T(C_Ast_InitDeclarator, decl->d_var.others, i);
                ec_collect_symbol_deps_ty(dec->ty, deps, f_scope, owner_name);
                if (dec->initializer) {
                    unimplemented();
                }
            }
        }
        break;
    }
    case C_AST_DECL_KIND_FN_DEF: {
        unreachable();
        break;
    }
    case C_AST_DECL_KIND_EMPTY:
        break;
    
    default:
        unreacheble();
        break;
    }
}