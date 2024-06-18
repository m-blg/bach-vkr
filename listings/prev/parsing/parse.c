
ParsingError
c_parse_ident(ParserState *state, C_Ast_Ident **out_ident) {
    C_Token *tok = parser_peek(state);
    if (tok->kind != C_TOKEN_KIND_IDENT) {
        return PARSING_ERROR(NONE);
    }

    make_node(state, out_ident, IDENT, 
            .name = tok->t_ident.name,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1}
        );
    parser_advance(state);
    return PARSING_ERROR(OK);
}

ParsingError
c_parse_declarator(ParserState *state, C_Ast_Type **decl_ty_head, C_Ast_Type **decl_ty_leaf, C_Ast_Ident **decl_name);
ParsingError
c_parse_direct_declarator(ParserState *state, C_Ast_Type **decl_ty_head, C_Ast_Type **decl_ty_leaf, C_Ast_Ident **decl_name);
ParsingError
c_parse_record(ParserState *state, C_Ast_TypeKind struct_or_union_kind, C_Ast_TypeRecord **out_rec);
ParsingError
c_parse_type_specifier(ParserState *state, C_Ast_Type **out_ty);
ParsingError
c_parse_declaration(ParserState *state, C_Ast_Decl **out_decl);

/// @param[in, out] state
/// @param[out] decl_ty_head
/// @param[out] decl_ty_leaf
/// @param[out] decl_name
ParsingError
c_parse_direct_declarator(ParserState *state, C_Ast_Type **decl_ty_head, C_Ast_Type **decl_ty_leaf, C_Ast_Ident **decl_name) {
    C_Token *tok;
    auto prev = parser_save(state);

    // TODO: figure out recursive type building
    C_Ast_Type *_decl_ty_head = nullptr, 
               *_decl_ty_leaf = nullptr;

    if (IS_OK(c_parse_ident(state, decl_name))) 
    {
        *decl_ty_head = nullptr;
        *decl_ty_leaf = nullptr;
        PARSING_OK();
    } 
    else if (IS_OK(c_parse_punct(state, C_PUNCT_LEFT_PAREN, &tok))) 
    {
        if (IS_ERR(c_parse_declarator(state, &_decl_ty_head, &_decl_ty_leaf, decl_name)) ||
            IS_ERR(c_parse_punct(state, C_PUNCT_RIGHT_PAREN, &tok))) 
        {
            PARSING_NONE(state, &prev);
        }
    }

    // [], ()
    if (IS_OK(c_parse_punct(state, C_PUNCT_LEFT_BRACKET, &tok))) {
        unimplemented();
        if (IS_ERR(c_parse_punct(state, C_PUNCT_RIGHT_BRACKET, &tok))) {
            PARSING_NONE(state, &prev);
        }
    }
    else if (IS_OK(c_parse_punct(state, C_PUNCT_LEFT_PAREN, &tok))) {
        unimplemented();
        if (IS_ERR(c_parse_punct(state, C_PUNCT_RIGHT_PAREN, &tok))) {
            PARSING_NONE(state, &prev);
        }
    }


    *decl_ty_head = _decl_ty_head;
    *decl_ty_leaf = _decl_ty_leaf;
    PARSING_OK();
}

#ifndef NDEBUG
#define DBG_ASSERT(x) ASSERT(x)
#else
#define DBG_ASSERT(x) 
#endif

INLINE
void
c_ast_type_append(C_Ast_Type *node, C_Ast_Type *leaf) {
    switch (node->ty_kind)
    {
    case C_AST_TYPE_KIND_POINTER:
        node->ty_pointer.pointee = leaf;
        break;
    case C_AST_TYPE_KIND_ARRAY:
        node->ty_array.item = leaf;
        break;
    case C_AST_TYPE_KIND_FUNCTION:
        unimplemented(); // TODO
        break;

    default:
        unreacheble();
        break;
    } 
}

/// @param[in, out] state
/// @param[out] decl_ty_head
/// @param[out] decl_ty_leaf
/// @param[out] decl_name
ParsingError
c_parse_declarator(ParserState *state, C_Ast_Type **decl_ty_head, C_Ast_Type **decl_ty_leaf, C_Ast_Ident **decl_name) {
    C_Token *tok;
    auto prev = parser_save(state);

    C_Ast_Type *pointer_decl_ty_head = nullptr, 
               *pointer_decl_ty_leaf = nullptr,
               *ty = nullptr;
    // pointer
    while (IS_OK(c_parse_punct(state, C_PUNCT_STAR, &tok))) {
        // make_node(state, TYPE_NAME, );
        ASSERT_OK(allocator_alloc_T(&state->ast_alloc, C_Ast_Type, &ty));
        *ty = (C_Ast_Type) {
            .kind = C_AST_NODE_KIND_TYPE_NAME,
            .ty_pointer = (C_Ast_TypePointer) {
                .ty_kind = C_AST_TYPE_KIND_POINTER,
                .pointee = nullptr,
            },
        };
        if (pointer_decl_ty_head == nullptr) {
            pointer_decl_ty_head = ty;
            pointer_decl_ty_leaf = ty;
        } else {
            ty->ty_pointer.pointee = pointer_decl_ty_head;
            pointer_decl_ty_head = ty;
        }

        // c_parse_qualifier_list();
    }

    C_Ast_Type *dir_decl_ty_head = nullptr, 
               *dir_decl_ty_leaf = nullptr;
    if (IS_ERR(c_parse_direct_declarator(state, &dir_decl_ty_head, &dir_decl_ty_leaf, decl_name))) {
        PARSING_NONE(state, &prev);
    }

    if (dir_decl_ty_head == nullptr) {
        dir_decl_ty_head = pointer_decl_ty_head;
    } else {
        c_ast_type_append(dir_decl_ty_leaf, pointer_decl_ty_head);
    }

    *decl_ty_head = dir_decl_ty_head;
    *decl_ty_leaf = pointer_decl_ty_leaf;
    DBG_ASSERT(*decl_name != nullptr);

    PARSING_OK();
}

ParsingError
c_parse_record(ParserState *state, C_Ast_TypeKind struct_or_union_kind, C_Ast_TypeRecord **out_rec) {
    C_Token *tok;

    C_Ast_Ident *name = nullptr;
    darr_T(C_Ast_Decl) fields = nullptr;

    auto prev = parser_save(state);

    if (IS_ERR(c_parse_ident(state, &name))) {
        PARSING_NONE(state, &prev);
    }

    if (c_parse_punct(state, C_PUNCT_LEFT_BRACE, &tok)) {
        allocator_free(&state->ast_alloc, (void **)&name);
        PARSING_NONE(state, &prev);
    }

    ASSERT_OK(darr_new_cap_in_T(C_Ast_Decl, 16, &state->ast_alloc, &fields));

    C_Ast_Decl *field;
    while (IS_OK(c_parse_declaration(state, &field))) {
        darr_push(&fields, field);
        allocator_free(&state->ast_alloc, (void **)&field);
    }

    if (c_parse_punct(state, C_PUNCT_RIGHT_BRACE, &tok)) {
        allocator_free(&state->ast_alloc, (void **)&name);
        darr_free(&fields);
        PARSING_NONE(state, &prev);
    }


    make_node_type(state, out_rec, STRUCT, 
        .ty_kind = struct_or_union_kind,
        .name = name,
        .fields = fields,
        .span = parser_span_from_save(state, &prev));

    PARSING_OK();
}

// declaration:
    // declaration-specifiers init-declarator-list? ;
// declaration-specifiers:
    // storage-class-specifier declaration-specifiers?
    // type-specifier declaration-specifiers?
    // type-qualifier declaration-specifiers?
    // function-specifier declaration-specifiers?
// init-declarator-list:
    // init-declarator
    // init-declarator-list , init-declarator
// init-declarator:
    // declarator
    // declarator = initializer

ParsingError
c_parse_type_specifier(ParserState *state, C_Ast_Type **out_ty) {
    // unimplemented();
    auto prev = parser_save(state);
    C_Token *tok = parser_peek(state);

    if (tok->kind == C_TOKEN_KIND_KEYWORD) {
        if (!c_keyword_is_type_specifier(tok->t_keyword.keyword_kind)) {
            if (tok->t_keyword.keyword_kind != C_KEYWORD_STRUCT && tok->t_keyword.keyword_kind != C_KEYWORD_UNION) {
                PARSING_NONE(state, &prev);
            }

            // struct-or-union-specifier
            if (IS_OK(c_parse_keyword(state, C_KEYWORD_STRUCT, &tok))) {
                if (IS_ERR(c_parse_record(state, C_AST_TYPE_KIND_STRUCT, (C_Ast_TypeRecord **)out_ty))) {
                    PARSING_NONE(state, &prev);
                }
            }
            else if (IS_OK(c_parse_keyword(state, C_KEYWORD_UNION, &tok))) {
                if (IS_ERR(c_parse_record(state, C_AST_TYPE_KIND_UNION, (C_Ast_TypeRecord **)out_ty))) {
                    PARSING_NONE(state, &prev);
                }
            }
        } else {
            make_node_type(state, (C_Ast_TypeIdent **)out_ty, IDENT, 
                .name = c_keyword_str_from_kind(tok->t_keyword.keyword_kind),
                .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
            parser_advance(state);
        }
    } else if (tok->kind == C_TOKEN_KIND_IDENT) {
        make_node_type(state, (C_Ast_TypeIdent **)out_ty, IDENT, 
            .name = tok->t_ident.name,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
        parser_advance(state);
    } else {
        PARSING_NONE(state, &prev);
    }

    PARSING_OK();
}

ParsingError
c_parse_declaration(ParserState *state, C_Ast_Decl **out_decl) {
    // unimplemented();

    C_Ast_InitDeclarator first = { };
    darr_t others;

    C_Token *tok;
    auto prev = parser_save(state);
    
    // declaration-specifiers
    // TODO specifiers qualifiers
    C_Ast_Type *ty = nullptr;
    if (IS_ERR(c_parse_type_specifier(state, &ty))) {
        parser_error(state, S("Expected type-specifier"));
        PARSING_NONE(state, &prev);
    }
    if (IS_OK(c_parse_punct(state, C_PUNCT_SEMI_COLON, &tok))) {
        first.ty = ty;
        goto out;
    }

    C_Ast_Type *decl_ty = nullptr,
               *decl_ty_leaf = nullptr;
    C_Ast_Ident *decl_name = nullptr;

    // init-declarator-list
    if (IS_ERR(c_parse_declarator(state, &decl_ty, &decl_ty_leaf, &decl_name))) {
        parser_error(state, S("Expected type-specifier"));
        PARSING_NONE(state, &prev);
    }
    if (IS_OK(c_parse_punct(state, C_PUNCT_EQUAL, &tok))) {
        unimplemented(); // TODO
        // c_parse_initializer(state, );
    }

    if (decl_ty) {
        c_ast_type_append(decl_ty_leaf, ty);
    } else {
        decl_ty = ty;
    }
    first = (C_Ast_InitDeclarator) {
        .ty = decl_ty,
        .name = decl_name,
    };

    if (IS_OK(c_parse_punct(state, C_PUNCT_COMMA, &tok))) {
        darr_new_cap_in_T(C_Ast_InitDeclarator, 3, &state->ast_alloc, &others);
        while (true) {
            // init-declarator
            if (IS_ERR(c_parse_declarator(state, &decl_ty, &decl_ty_leaf, &decl_name))) {
                parser_error(state, S("Expected type-specifier"));
                darr_free(&others);
                PARSING_NONE(state, &prev);
            }

            if (IS_OK(c_parse_punct(state, C_PUNCT_EQUAL, &tok))) {
                unimplemented(); // TODO
                // c_parse_initializer(state, );
            }

            if (decl_ty) {
                c_ast_type_append(decl_ty_leaf, ty);
            } else {
                decl_ty = ty;
            }
            darr_push(&others, &(C_Ast_InitDeclarator) {
                .ty = decl_ty,
                .name = decl_name,
            });

            if (IS_ERR(c_parse_punct(state, C_PUNCT_COMMA, &tok))) {
                break;
            }
        }
    }
    if (IS_ERR(c_parse_punct(state, C_PUNCT_SEMI_COLON, &tok))) {
        parser_error(state, S("Expected ';'"));
        darr_free(&others);
        PARSING_NONE(state, &prev);
    }

out:
    make_node(state, out_decl, DECL, 
            .ty = first.ty,
            .name = first.name,
            .initializer = first.initializer,
            .others = others,
            .span = parser_span_from_save(state, &prev),
        );

    PARSING_OK();
}