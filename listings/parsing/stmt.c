ParsingError
c_parse_stmt(ParserState *state, C_Ast_Stmt **out_stmt) {
    // state->was_error = false;

    auto prev = parser_save(state);
    C_Token *tok = parser_peek(state);

    switch (tok->kind)
    {
    case C_TOKEN_KIND_CHAR:
    case C_TOKEN_KIND_STRING:
    case C_TOKEN_KIND_NUMBER:
        TRY(c_parse_stmt_expr(state, (C_Ast_StmtExpr **)out_stmt));
        break;
    case C_TOKEN_KIND_IDENT:
        tok = parser_peek2(state);
        if (tok->kind == C_TOKEN_KIND_PUNCT && tok->t_punct.punct_kind == C_PUNCT_COLON) {
            C_Ast_Ident *label = nullptr;
            c_parse_ident(state, &label);
            parser_advance(state);
            make_node_stmt(state, (C_Ast_StmtLabel **)out_stmt, LABEL, 
                .label = label,
                .span = parser_span_from_save(state, &prev));
        } else {
            TRY(c_parse_stmt_expr(state, (C_Ast_StmtExpr **)out_stmt));
        }

        break;
    case C_TOKEN_KIND_PUNCT:
        if (tok->t_punct.punct_kind == C_PUNCT_LEFT_BRACE) {
            // compound statement    
            parser_advance(state);
            // if (IS_ERR(c_parse_t_punct_kind(state, C_PUNCT_LEFT_BRACE, &tok))) {
            //     PARSING_NONE(state, &prev);
            // }

           darr_T(C_Ast_BlockItem*) items = nullptr; 
           C_SymbolTable scope = nullptr;
           PARSER_OPTIONAL(c_parse_block(state, &scope, &items));


            if (IS_ERR(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_BRACE, &tok))) {
                PARSING_NONE(state, &prev);
            }
            

            make_node_stmt(state, (C_Ast_StmtCompound **)out_stmt, COMPOUND, 
                .items = items,
                .scope = scope,
                .span = parser_span_from_save(state, &prev));
        } else {
            TRY(c_parse_stmt_expr(state, (C_Ast_StmtExpr **)out_stmt));
            break;
        }
        break;

    case C_TOKEN_KIND_KEYWORD:
        if (!c_keyword_is_control(tok->t_keyword.keyword_kind)) {
            TRY(c_parse_stmt_expr(state, (C_Ast_StmtExpr **)out_stmt));
            break;
        }

        tok = parser_advance(state);
        switch (tok->t_keyword.keyword_kind)
        {
        case C_KEYWORD_IF: {
            C_Ast_Expr *cond = nullptr;
            C_Ast_Stmt *then = nullptr,
                       *_else = nullptr;
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_LEFT_PAREN, &tok))
            PARSER_TRY(c_parse_expr(state, &cond));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));
            PARSER_TRY(c_parse_stmt(state, &then));
            
            if (IS_OK(PARSER_OPTIONAL(c_parse_t_keyword_kind(state, C_KEYWORD_ELSE, &tok)))) {
                PARSER_TRY(c_parse_stmt(state, &_else));
            }

            make_node_stmt(state, (C_Ast_StmtIf **)out_stmt, IF, 
                .e_cond = cond,
                .s_then = then,
                .s_else = _else,
                .span = parser_span_from_save(state, &prev));
            
            break;
        }
        case C_KEYWORD_SWITCH: {
            C_Ast_Expr *item = nullptr;
            C_Ast_Stmt *body = nullptr;
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_LEFT_PAREN, &tok))
            PARSER_TRY(c_parse_expr(state, &item));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));
            PARSER_TRY(c_parse_stmt(state, &body));

            make_node_stmt(state, (C_Ast_StmtSwitch **)out_stmt, SWITCH, 
                .e_item = item,
                .s_body = body,
                .span = parser_span_from_save(state, &prev));
            break;
        }
        case C_KEYWORD_CASE: {
            C_Ast_Expr *item = nullptr;
            PARSER_TRY(c_parse_expr(state, &item));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_COLON, &tok));

            make_node_stmt(state, (C_Ast_StmtCase **)out_stmt, CASE, 
                .e_item = item,
                .span = parser_span_from_save(state, &prev));
            break;
        }
        case C_KEYWORD_DEFAULT: {
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_COLON, &tok));

            make_node_stmt(state, (C_Ast_StmtDefault **)out_stmt, DEFAULT, 
                .span = parser_span_from_save(state, &prev));
            break;
        }
        case C_KEYWORD_FOR: {
            C_Ast_Decl *vars = nullptr;
            C_Ast_Expr *cond = nullptr,
                       *step = nullptr;
            C_Ast_Stmt *body = nullptr;
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_LEFT_PAREN, &tok))
            PARSER_TRY(c_parse_decl(state, &vars));
            if (vars->decl_kind == C_AST_DECL_KIND_EMPTY) {
                allocator_free(&state->ast_alloc, (void **)&vars);
            } else if (vars->decl_kind != C_AST_DECL_KIND_VARIABLE) {
                parser_error_expected(state, S("variable declaration"));
                PARSING_NONE(state, &prev);
            }
            // decl parsed ';'
            PARSER_OPTIONAL(c_parse_expr(state, &cond));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok));
            PARSER_OPTIONAL(c_parse_expr(state, &step));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));
            PARSER_TRY(c_parse_stmt(state, &body));

            make_node_stmt(state, (C_Ast_StmtFor **)out_stmt, FOR, 
                .d_vars = vars,
                .e_cond = cond,
                .e_step = step,
                .s_body = body,
                .span = parser_span_from_save(state, &prev));
            break;
        }
        case C_KEYWORD_WHILE: {
            C_Ast_Expr *cond = nullptr;
            C_Ast_Stmt *body = nullptr;

            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_LEFT_PAREN, &tok))
            PARSER_TRY(c_parse_expr(state, &cond));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));
            PARSER_TRY(c_parse_stmt(state, &body));

            make_node_stmt(state, (C_Ast_StmtWhile **)out_stmt, WHILE, 
                .e_cond = cond,
                .s_body = body,
                .span = parser_span_from_save(state, &prev));
            
            break;
        }
        case C_KEYWORD_DO: {
            C_Ast_Stmt *body = nullptr;
            C_Ast_Expr *cond = nullptr;

            PARSER_TRY(c_parse_stmt(state, &body));
            PARSER_TRY(c_parse_t_keyword_kind(state, C_KEYWORD_WHILE, &tok))
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_LEFT_PAREN, &tok))
            PARSER_TRY(c_parse_expr(state, &cond));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok));

            make_node_stmt(state, (C_Ast_StmtDoWhile **)out_stmt, DO_WHILE, 
                .s_body = body,
                .e_cond = cond,
                .span = parser_span_from_save(state, &prev));
            
            break;
        }
        case C_KEYWORD_GOTO: {
            C_Ast_Ident *label = nullptr;
            PARSER_TRY(c_parse_ident(state, &label));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok));

            make_node_stmt(state, (C_Ast_StmtGoto **)out_stmt, GOTO, 
                .label = label,
                .span = parser_span_from_save(state, &prev));
            break;
        }
        case C_KEYWORD_BREAK: {
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok));
            make_node_stmt(state, (C_Ast_StmtBreak **)out_stmt, BREAK, 
                .span = parser_span_from_save(state, &prev));
            break;
        }
        case C_KEYWORD_CONTINUE: {
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok));
            make_node_stmt(state, (C_Ast_StmtContinue **)out_stmt, CONTINUE, 
                .span = parser_span_from_save(state, &prev));
            break;
        }
        case C_KEYWORD_RETURN: {
            C_Ast_Expr *expr = nullptr;
            PARSER_OPTIONAL(c_parse_expr(state, &expr));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_SEMI_COLON, &tok));

            make_node_stmt(state, (C_Ast_StmtReturn **)out_stmt, RETURN, 
                .e_ret = expr,
                .span = parser_span_from_save(state, &prev));
            break;
        }
        
        default:
            TRY(c_parse_stmt_expr(state, (C_Ast_StmtExpr **)out_stmt));
            break;
        }
        break;

    case C_TOKEN_KIND_EOF:
        PARSING_NONE(state, &prev);
    
    case C_TOKEN_KIND_COMMENT:
    case C_TOKEN_KIND_NEW_LINE:
    case C_TOKEN_KIND_PP_DIRECTIVE:
    case C_TOKEN_KIND_INCLUDE:
    case C_TOKEN_KIND_EXPAND:
        unreacheble();
        break;
    default:
        unreacheble();
        break;
    }

    PARSING_OK(state);
}

bool
c_scope_add_symbol(C_SymbolTable *scope, C_Symbol sym, C_SymbolData *data) {
    if (hashmap_get(*scope, &sym) != nullptr) {
        return false;
    }
    ASSERT_OK(hashmap_set(scope, &sym, data));
    return true;
}

bool
c_parser_scope_collect_symbols_decl(ParserState *state, C_SymbolTable *scope, C_Ast_Decl *decl) {
    #define abort(name) { \
        parser_error_pos(state, parser_pos_from_t_index(state, decl->span.b_tok), S("redefinition of "KMAG"%s"KNRM), (name)); \
        return false; \
    }
    switch (decl->decl_kind)
    {
    case C_AST_DECL_KIND_TYPEDEF: {
        if (!c_scope_add_symbol(scope, decl->d_typedef.name->name, 
            &(C_SymbolData) {
                .node = (C_Ast_Node *)decl,
            })) 
        {
            abort(decl->d_typedef.name->name);
        }
        
        if (decl->d_typedef.others) {
            for_in_range(i, 0, darr_len(decl->d_typedef.others)) {
                auto dec = darr_get_T(C_Ast_Declarator, decl->d_typedef.others, i);
                if (!c_scope_add_symbol(scope, dec->name->name, 
                    &(C_SymbolData) {
                        .node = (C_Ast_Node *)decl,
                    })) 
                {
                    abort(dec->name->name);
                }
            }
        }
        break;
    }
    case C_AST_DECL_KIND_TYPE_DECL: {
        auto ty = decl->d_type.ty;
        switch (ty->ty_kind)
        {
        case C_AST_TYPE_KIND_STRUCT:
            if (ty->ty_struct.name) {
                str_t name = ty->ty_struct.name->name;
                String s;
                PARSER_ALLOC_HANDLE(string_new_cap_in(str_len(S("struct ")) + str_len(name), &state->string_alloc, &s));
                sprint_fmt(&s, S("struct %s"), name);
                if (!c_scope_add_symbol(scope, string_to_str(&s), 
                    &(C_SymbolData) {
                        .node = (C_Ast_Node *)decl,
                    })) 
                {
                    abort(string_to_str(&s));
                }
            }
            break;
        case C_AST_TYPE_KIND_UNION:
            if (ty->ty_union.name) {
                str_t name = ty->ty_union.name->name;
                String s;
                PARSER_ALLOC_HANDLE(string_new_cap_in(str_len(S("union ")) + str_len(name), &state->string_alloc, &s));
                sprint_fmt(&s, S("union %s"), name);
                if (!c_scope_add_symbol(scope, string_to_str(&s), 
                    &(C_SymbolData) {
                        .node = (C_Ast_Node *)decl,
                    })) 
                {
                    abort(string_to_str(&s));
                }
            }
            break;
        case C_AST_TYPE_KIND_ENUM:
            if (ty->ty_enum.name) {
                str_t name = ty->ty_enum.name->name;
                String s;
                PARSER_ALLOC_HANDLE(string_new_cap_in(str_len(S("enum ")) + str_len(name), &state->string_alloc, &s));
                sprint_fmt(&s, S("enum %s"), name);
                if (!c_scope_add_symbol(scope, string_to_str(&s), 
                    &(C_SymbolData) {
                        .node = (C_Ast_Node *)decl,
                    })) 
                {
                    abort(string_to_str(&s));
                }
            }
            break;
        
        default:
            goto out;
        }
        break;
    }
    case C_AST_DECL_KIND_VARIABLE: {
        if (!c_scope_add_symbol(scope, decl->d_var.name->name, 
            &(C_SymbolData) {
                .node = (C_Ast_Node *)decl,
            })) 
        {
            abort(decl->d_var.name->name);
        }
        
        if (decl->d_var.others) {
            for_in_range(i, 0, darr_len(decl->d_var.others)) {
                auto idec = darr_get_T(C_Ast_InitDeclarator, decl->d_var.others, i);
                if (!c_scope_add_symbol(scope, idec->name->name,
                    &(C_SymbolData) {
                        .node = (C_Ast_Node *)decl,
                    })) 
                {
                    abort(idec->name->name);
                }
            }
        }
        break;
    }
    case C_AST_DECL_KIND_FN_DEF: {
        if (!c_scope_add_symbol(scope, decl->d_fn_def.name->name, 
            &(C_SymbolData) {
                .node = (C_Ast_Node *)decl,
            })) 
        {
            abort(decl->d_fn_def.name->name);
        }
        break;
    }
    case C_AST_DECL_KIND_EMPTY:
        break;
    
    default:
        unreacheble();
        break;
    }
out:
    return true;
}

ParsingError
c_parse_block(ParserState *state, C_SymbolTable *scope, darr_T(C_Ast_BlockItem *) *out_items) {
    auto prev = parser_save(state);
    if (*scope == nullptr) {
        PARSER_ALLOC_HANDLE(hashmap_new_cap_in_T(C_Symbol, C_SymbolData, 8, &state->ast_alloc, scope));
    }
    darr_t items = nullptr;
    PARSER_ALLOC_HANDLE(darr_new_cap_in_T(C_Ast_BlockItem *, 8, &state->ast_alloc, &items));
    
    c_environment_push_scope(&state->env, scope);

    while (true) {
        if (parser_peek_punct_kind(state, C_PUNCT_RIGHT_BRACE)) {
            break;
        }
        if (parser_peek_punct_kind(state, C_PUNCT_SEMI_COLON) || 
            c_token_is_type_name_beginning(state->env, parser_peek(state))) 
        {
            C_Ast_Decl *block_item = nullptr;
            PARSER_TRY(c_parse_decl(state, &block_item))
            PARSER_ALLOC_HANDLE(darr_push(&items, &block_item));
            if (state->collect_symbols) {
                if (!c_parser_scope_collect_symbols_decl(state, scope, block_item)) {
                    PARSING_NONE(state, &prev);
                }
            }
        } else {
            C_Ast_Stmt *block_item = nullptr;
            PARSER_TRY(c_parse_stmt(state, &block_item))
            PARSER_ALLOC_HANDLE(darr_push(&items, &block_item));
        }
    }

    c_environment_pop_scope(&state->env);
    *out_items = items;

    PARSING_OK(state);
}