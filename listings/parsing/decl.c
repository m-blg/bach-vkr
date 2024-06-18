
c_parse_type_specifier(ParserState *state, C_Ast_Type **out_ty) {
    // unimplemented();
    auto prev = parser_save(state);
    C_Token *tok = parser_peek(state);

    if (tok->kind == C_TOKEN_KIND_KEYWORD) {
        if (!c_token_is_type_name_beginning(state->env, tok)) {
            parser_error_expected(state, S("type-name"));
            PARSING_NONE(state, &prev);
        }
        if (!c_keyword_is_type_specifier(tok->t_keyword.keyword_kind)) {
            if (tok->t_keyword.keyword_kind != C_KEYWORD_STRUCT && tok->t_keyword.keyword_kind != C_KEYWORD_UNION) {
                PARSING_NONE(state, &prev);
            }

            // struct-or-union-specifier
            if (IS_OK(PARSER_OPTIONAL(c_parse_t_keyword_kind(state, C_KEYWORD_STRUCT, &tok)))) {
                if (IS_ERR(c_parse_record(state, C_AST_TYPE_KIND_STRUCT, (C_Ast_TypeRecord **)out_ty))) {
                    PARSING_NONE(state, &prev);
                }
            }
            else if (IS_OK(PARSER_OPTIONAL(c_parse_t_keyword_kind(state, C_KEYWORD_UNION, &tok)))) {
                if (IS_ERR(c_parse_record(state, C_AST_TYPE_KIND_UNION, (C_Ast_TypeRecord **)out_ty))) {
                    PARSING_NONE(state, &prev);
                }
            }
        } else {
            C_Ast_Ident *ident = nullptr;
            make_node(state, &ident, IDENT, 
                .name = c_keyword_str_from_kind(tok->t_keyword.keyword_kind),
                .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
            make_node_type(state, (C_Ast_TypeIdent **)out_ty, IDENT, 
                .ident = ident,
                .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
            parser_advance(state);
        }
    } else if (tok->kind == C_TOKEN_KIND_IDENT) {
        if (!c_token_is_type_name_beginning(state->env, tok)) {
            parser_error_expected(state, S("type-name"));
            PARSING_NONE(state, &prev);
        }

        C_Ast_Ident *ident = nullptr;
        make_node(state, &ident, IDENT, 
            .name = tok->t_ident.name,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
        make_node_type(state, (C_Ast_TypeIdent **)out_ty, IDENT, 
            .ident = ident,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
        parser_advance(state);
    } else {
        parser_error_expected(state, S("type-specifier"));
        PARSING_NONE(state, &prev);
    }

    PARSING_OK(state);
}

ParsingError
c_parse_declaration(ParserState *state, C_Ast_Decl **out_decl) {
    // unimplemented();

    C_Ast_InitDeclarator first = { };
    darr_t others = nullptr;

    C_Token *tok;
    auto prev = parser_save(state);

    if (IS_OK(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok)))) {
        make_node_decl(state, (C_Ast_Decl **)out_decl, EMPTY, 
                .span = parser_span_from_save(state, &prev),
            );
        PARSING_OK(state);
    }

    C_Ast_DeclKind decl_kind = C_AST_DECL_KIND_VARIABLE;
    if (IS_OK(PARSER_OPTIONAL(c_parse_t_keyword_kind(state, C_KEYWORD_TYPEDEF, &tok)))) {
        decl_kind = C_AST_DECL_KIND_TYPEDEF;
    }
    
    // declaration-specifiers
    // TODO specifiers qualifiers
    C_Ast_Type *ty = nullptr;
    if (IS_ERR(c_parse_type_specifier(state, &ty))) {
        // parser_error(state, S("Expected type-specifier"));
        PARSING_NONE(state, &prev);
    }
    if (IS_OK(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok)))) {
        make_node_decl(state, (C_Ast_DeclType **)out_decl, TYPE_DECL, 
                .ty = ty,
                .span = parser_span_from_save(state, &prev),
            );
        PARSING_OK(state);
    }

    C_Ast_Type NLB(*)decl_ty = nullptr,
               NLB(*)decl_ty_leaf = nullptr;
    C_Ast_Ident *decl_name = nullptr;

    // init-declarator-list
    if (IS_ERR(c_parse_declarator(state, &decl_ty, &decl_ty_leaf, &decl_name))) {
        // parser_error(state, S("Expected type-specifier"));
        PARSING_NONE(state, &prev);
    }
    if (decl_ty) {
        c_ast_type_append(decl_ty_leaf, ty);
    } else {
        decl_ty = ty;
    }

    C_Ast_Expr *initializer = nullptr;

    auto save_br = parser_save(state);
    if (decl_ty->ty_kind == C_AST_TYPE_KIND_FUNCTION && 
        IS_OK(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_LEFT_BRACE, &tok)))) 
    {

        C_Ast_StmtCompound *body = nullptr;

        darr_T(C_Ast_BlockItem*) items = nullptr; 
        C_SymbolTable scope = nullptr;
        PARSER_TRY(c_parse_block(state, &scope, &items));

        if (IS_ERR(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_BRACE, &tok))) {
            PARSING_NONE(state, &prev);
        }
        

        make_node_stmt(state, (C_Ast_StmtCompound **)&body, COMPOUND, 
            .items = items,
            .scope = scope,
            .span = parser_span_from_save(state, &save_br));

        // if (parser_peek(state)->kind != C_TOKEN_KIND_PUNCT || 
        //     parser_peek(state)->t_punct.punct_kind != C_PUNCT_RIGHT_BRACE) 
        // {
        //     unimplemented();
        // }
        // if (IS_ERR(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_BRACE, &tok))) {
        //     PARSING_NONE(state, &prev);
        // }

        make_node_decl(state, (C_Ast_DeclFnDef **)out_decl, FN_DEF, 
                .ty = decl_ty,
                .name = decl_name,
                .body = body,
                // .others = others,
                .span = parser_span_from_save(state, &prev),
            );
        PARSING_OK(state);

    } else if (IS_OK(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_EQUAL, &tok)))) {
        PARSER_TRY(c_parse_expr_assign(state, &initializer));
    }

    first = (C_Ast_InitDeclarator) {
        .ty = decl_ty,
        .name = decl_name,
        .initializer = initializer,
    };

    if (IS_OK(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_COMMA, &tok)))) {
        darr_new_cap_in_T(C_Ast_InitDeclarator, 3, &state->ast_alloc, &others);
        while (true) {
            C_Ast_Expr *initializer = nullptr;
            // init-declarator
            if (IS_ERR(c_parse_declarator(state, &decl_ty, &decl_ty_leaf, &decl_name))) {
                parser_error(state, S("Expected type-specifier"));
                darr_free(&others);
                PARSING_NONE(state, &prev);
            }

            if (IS_OK(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_EQUAL, &tok)))) {
                PARSER_TRY(c_parse_expr_assign(state, &initializer));
            }

            if (decl_ty) {
                c_ast_type_append(decl_ty_leaf, ty);
            } else {
                decl_ty = ty;
            }
            darr_push(&others, &(C_Ast_InitDeclarator) {
                .ty = decl_ty,
                .name = decl_name,
                .initializer = initializer,
            });

            if (IS_ERR(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_COMMA, &tok)))) {
                break;
            }
        }
    }
    if (IS_ERR(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok))) {
        // parser_error(state, S("Expected ';'"));
        if (others) {
            darr_free(&others);
        }
        PARSING_NONE(state, &prev);
    }

    make_node_decl(state, (C_Ast_DeclVar **)out_decl, INVALID, 
            .decl_kind = decl_kind,
            .ty = first.ty,
            .name = first.name,
            .initializer = first.initializer,
            .others = others,
            .span = parser_span_from_save(state, &prev),
        );

    PARSING_OK(state);
}