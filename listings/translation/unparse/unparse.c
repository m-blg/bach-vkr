FmtError
c_ast_expr_unparse_fmt(C_Ast_Expr *expr, StringFormatter *fmt, bool force_parens) {
    if (force_parens || bitfield_is_flag_set(&expr->flags, C_EXPR_FLAG_IN_PARENS)) {
        TRY(string_formatter_write(fmt, S("(")));
    }

    switch (expr->expr_kind)
    {
    case C_AST_EXPR_KIND_IDENT:
        TRY(c_ast_ident_unparse_fmt(expr->e_ident.ident, fmt, nullptr));
        break;
    case C_AST_EXPR_KIND_LITERAL:
        TRY(c_ast_literal_unparse_fmt(expr->e_lit.lit, fmt, nullptr));
        break;
    case C_AST_EXPR_KIND_BINOP: {
        TRY(c_ast_expr_unparse_fmt(expr->e_bin_op.e_lhs, fmt, force_parens));
        if (expr->e_bin_op.op == C_OPERATOR_DOT || expr->e_bin_op.op == C_OPERATOR_ARROW) {
            TRY(string_formatter_write_fmt(fmt, S("%s"), c_operator_data(expr->e_bin_op.op).sym));
        } else {
            TRY(string_formatter_write_fmt(fmt, S(" %s "), c_operator_data(expr->e_bin_op.op).sym));
        }
        TRY(c_ast_expr_unparse_fmt(expr->e_bin_op.e_rhs, fmt, force_parens));
        break;
    }
    case C_AST_EXPR_KIND_UNOP: {
        if (bitfield_is_flag_set(&expr->flags, C_EXPR_FLAG_IS_PREFIX)) {
            TRY(string_formatter_write_fmt(fmt, S("%s"), c_operator_data(expr->e_un_op.op).sym));
            TRY(c_ast_expr_unparse_fmt(expr->e_un_op.e_operand, fmt, force_parens));
        } else {
            TRY(c_ast_expr_unparse_fmt(expr->e_un_op.e_operand, fmt, force_parens));
            TRY(string_formatter_write_fmt(fmt, S("%s"), c_operator_data(expr->e_un_op.op).sym));
        }
        break;
    }
    case C_AST_EXPR_KIND_CAST: {
        TRY(string_formatter_write(fmt, S("(")));
        TRY(c_ast_type_unparse_fmt(expr->e_cast.ty, fmt, nullptr, false));
        TRY(string_formatter_write(fmt, S(") ")));
        TRY(c_ast_expr_unparse_fmt(expr->e_cast.e_rhs, fmt, force_parens));
        break;
    }
    case C_AST_EXPR_KIND_FN_CALL: {
        TRY(c_ast_expr_unparse_fmt(expr->e_fn_call.caller, fmt, force_parens));
        TRY(string_formatter_write(fmt, S("(")));
        if (expr->e_fn_call.args) {
            TRY(c_ast_expr_unparse_fmt(
                *darr_get_T(C_Ast_Expr *, expr->e_fn_call.args, 0), fmt, force_parens));
            for_in_range(i, 1, darr_len(expr->e_fn_call.args)) {
                TRY(string_formatter_write(fmt, S(", ")));
                TRY(c_ast_expr_unparse_fmt(
                    *darr_get_T(C_Ast_Expr *, expr->e_fn_call.args, i), fmt, force_parens));
            }
        }
        TRY(string_formatter_write(fmt, S(")")));

        break;
    }
    case C_AST_EXPR_KIND_ARRAY_SUB: {
        TRY(c_ast_expr_unparse_fmt(expr->e_array_sub.array, fmt, force_parens));
        TRY(string_formatter_write(fmt, S("[")));
        if (expr->e_array_sub.arg) {
            TRY(c_ast_expr_unparse_fmt(expr->e_array_sub.arg, fmt, force_parens));
        }
        TRY(string_formatter_write(fmt, S("]")));
        break;
    }
    case C_AST_EXPR_KIND_CONDOP: {
        TRY(c_ast_expr_unparse_fmt(expr->e_cond_op.e_cond, fmt, force_parens));
        TRY(string_formatter_write(fmt, S(" ? ")));
        TRY(c_ast_expr_unparse_fmt(expr->e_cond_op.e_then, fmt, force_parens));
        TRY(string_formatter_write(fmt, S(" : ")));
        TRY(c_ast_expr_unparse_fmt(expr->e_cond_op.e_else, fmt, force_parens));
        break;
    }
    case C_AST_EXPR_KIND_COMPOUND: {
        TRY(c_ast_expr_unparse_fmt(
            *darr_get_T(C_Ast_Expr *, expr->e_compound.items, 0), fmt, force_parens));
        for_in_range(i, 1, darr_len(expr->e_compound.items)) {
            TRY(string_formatter_write(fmt, S(", ")));
            TRY(c_ast_expr_unparse_fmt(
                *darr_get_T(C_Ast_Expr *, expr->e_compound.items, i), fmt, force_parens));
        }
        break;
    }
    
    default:
        unreacheble();
        break;
    }

    if (force_parens || bitfield_is_flag_set(&expr->flags, C_EXPR_FLAG_IN_PARENS)) {
        TRY(string_formatter_write(fmt, S(")")));
    }

    return FMT_ERROR(OK);
}


FmtError
c_ast_type_unparse_fmt(C_Ast_Type *ty, StringFormatter *fmt, void *var_name, bool discard_leaf) {
    String s, temp;
    str_t temp_str;
    C_Ast_TypeKind *prev_ty_kind = nullptr;
    C_Ast_TypeKind type_kind_ident = C_AST_TYPE_KIND_IDENT;
    StringFormatter temp_fmt;
    
    // NOTE: can customize error hander in ASSERT
    ASSERT_OK(string_new_cap_in(64, &g_ctx.global_alloc, &s));
    ASSERT_OK(string_new_cap_in(64, &g_ctx.global_alloc, &temp));
    string_formatter_init_string_default(&temp_fmt, &temp);

    if (var_name) {
        string_append_str(&s, *(str_t *)var_name);
        prev_ty_kind = &type_kind_ident;
    }

    while (true) {
        switch (ty->ty_kind)
        {
        case C_AST_TYPE_KIND_IDENT:
            if (discard_leaf) {
                goto out;
            }
            if (prev_ty_kind == nullptr) {
                string_append_str(&s, ty->ty_ident.ident->name);
            } else {
                string_prepend_str(&s, S(" "));
                string_prepend_str(&s, ty->ty_ident.ident->name);
            }
            goto out;
            break;
        case C_AST_TYPE_KIND_POINTER:
            string_prepend_str(&s, S("*"));
            prev_ty_kind = &ty->ty_kind;
            ty = ty->ty_pointer.pointee;
            continue;
            break;
        case C_AST_TYPE_KIND_ARRAY:
            if (ty->ty_array.count != nullptr) {
                // ASSERT_OK(string_new_cap_in(64, &g_ctx.global_alloc, &temp));
                string_reset(&temp);
                TRY(c_ast_expr_unparse_fmt(ty->ty_array.count, &temp_fmt, nullptr));
                temp_str = string_to_str(&temp);
            } else {
                temp_str = S("");
            }

            if (prev_ty_kind == nullptr || *prev_ty_kind == C_AST_TYPE_KIND_ARRAY 
                                   || *prev_ty_kind == C_AST_TYPE_KIND_IDENT) 
            {
                string_append_str(&s, S("["));
                string_append_str(&s, temp_str);
                string_append_str(&s, S("]"));
            } else {
                string_prepend_str(&s, S("("));
                // string_append_str_fmt(&s, S(")[%s]"), string_to_str(&temp));
                string_append_str(&s, S(")["));
                string_append_str(&s, temp_str);
                string_append_str(&s, S("]"));
            }

            // if (ty->ty_array.count != nullptr) {
            //     string_free(&temp);
            // }
            
            prev_ty_kind = &ty->ty_kind;
            ty = ty->ty_array.item;
            break;
        case C_AST_TYPE_KIND_FUNCTION:
            if (prev_ty_kind != nullptr && *prev_ty_kind != C_AST_TYPE_KIND_IDENT) 
            {
                string_prepend_str(&s, S("("));
                string_append_str(&s, S(")"));
            }

            string_append_str(&s, S("("));
            if (ty->ty_fn.args) {
                for_in_range(i, 0, darr_len(ty->ty_fn.args)-1) {
                    string_reset(&temp);
                    auto arg = darr_get_T(C_Ast_FnParam, ty->ty_fn.args, i);
                    TRY(c_ast_type_unparse_fmt(arg->ty, &temp_fmt, &arg->name->name, false));
                    string_append_str(&s, string_to_str(&temp));
                    string_append_str(&s, S(", "));
                }
                {
                    string_reset(&temp);
                    auto arg = darr_get_iT(C_Ast_FnParam, ty->ty_fn.args, -1);
                    TRY(c_ast_type_unparse_fmt(arg->ty, &temp_fmt, &arg->name->name, false));
                    string_append_str(&s, string_to_str(&temp));
                }
            }
            string_append_str(&s, S(")"));


            prev_ty_kind = &ty->ty_kind;
            ty = ty->ty_fn.ret;
            break;
        case C_AST_TYPE_KIND_STRUCT:
            string_reset(&temp);
            TRY(c_ast_struct_unparse_fmt(&ty->ty_struct, &temp_fmt, nullptr));
            if (prev_ty_kind != nullptr) {
                string_append_str(&temp, S(" "));
                string_prepend_str(&s, string_to_str(&temp));
            } else {
                string_append_str(&s, string_to_str(&temp));
            }
            goto out;
            break;
        case C_AST_TYPE_KIND_UNION:
        case C_AST_TYPE_KIND_ENUM:
            unimplemented();
            break;
        // case C_AST_TYPE_KIND_POINTER:
        //     TRY(c_ast_type_unparse_fmt(ty->pointer.pointee, *fmt, nullptr));
        //     break;
        
        default:
            unreacheble();
            break;
        }
    }
out:

    TRY(string_formatter_write(fmt, string_to_str(&s)));
    string_free(&s);
    string_free(&temp);

    return FMT_ERROR(OK);
}


FmtError
c_ast_translation_unit_unparse_fmt(C_Ast_TranslationUnit *tr_unit, StringFormatter *fmt, void *_) {
    auto decls = tr_unit->decls;
    for_in_range(i, 0, darr_len(decls)) {
        auto decl = *darr_get_T(C_Ast_Decl *, decls, i);
        if (decl->decl_kind == C_AST_DECL_KIND_EMPTY) {
            continue;
        }
        TRY(c_ast_decl_unparse_fmt(decl, fmt, nullptr));
        TRY(string_formatter_write(fmt, S("\n\n")));
    }

    return FMT_ERROR(OK);
}