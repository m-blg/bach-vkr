ParsingError
_c_parse_expr(ParserState *state, C_Ast_Expr **out_expr, u8_t max_precedence, C_ExprFlags flags) {
    auto prev = parser_save(state);
    C_Token *tok = parser_peek(state);
    C_Ast_Expr *first = nullptr;

    switch (tok->kind)
    {
    case C_TOKEN_KIND_PUNCT: {
        if (tok->t_punct.punct_kind == C_PUNCT_LEFT_PAREN) {
            parser_advance(state);
            if (c_token_is_type_name_beginning(state->env, parser_peek(state))) {
                C_Ast_Type *ty = nullptr;
                PARSER_TRY(c_parse_type_specifier(state, &ty));
                PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));

                if (parser_peek_punct_kind(state, C_PUNCT_LEFT_BRACE)) {
                    // compound literal
                    darr_t items = nullptr;
                    PARSER_OPTIONAL(c_parse_lit_compound_body(state, &items));
                    C_Ast_LiteralCompound *lit;
                    make_node_lit(state, &lit, COMPOUND, 
                        .ty = ty,
                        .entries = items);
                    make_node_expr(state, (C_Ast_ExprLit **)&first, LITERAL,
                        .lit = (C_Ast_Literal *)lit);
                } else {
                    C_Ast_ExprCast *op;
                    // cast operator
                    make_node_expr(state, (C_Ast_ExprCast **)&op, CAST,
                        .ty = ty);

                    PARSER_TRY(_c_parse_expr(state, &op->e_rhs, c_operator_precedence(C_OPERATOR_CAST), C_EXPR_FLAG_EMPTY));
                    first = (C_Ast_Expr *)op;
                }
            } else {
                // parenthesized expression
                PARSER_TRY(_c_parse_expr(state, &first, C_MAX_PRECEDENCE, C_EXPR_FLAG_STRICT_PRECEDENCE | C_EXPR_FLAG_IN_PARENS));
                PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));
            }
        } else {
            // prefix operator
            C_Ast_ExprUnOp *op;
            if (IS_ERR(c_parse_expr_op_prefix_prec_rb(state, &op, max_precedence, bitfield_is_flag_set(&flags, C_EXPR_FLAG_STRICT_PRECEDENCE)))) {
                parser_error_expected(state, S("expression"));
                return PARSING_ERROR(NONE);
            }

            // parse increasing precedence
            PARSER_TRY(_c_parse_expr(state, &op->e_operand, c_operator_precedence(op->op), C_EXPR_FLAG_EMPTY));
            first = (C_Ast_Expr *)op;
        }
        break;
    }
    case C_TOKEN_KIND_IDENT: {
        C_Ast_Ident *ident;
        ASSERT_OK(c_parse_ident(state, &ident));
        make_node_expr(state, (C_Ast_ExprIdent **)&first, IDENT,
            .ident = ident,
            .span = parser_span_from_save(state, &prev));

        break;
    }
    case C_TOKEN_KIND_NUMBER: {
        C_Ast_LiteralNumber *lit;
        make_node_lit(state, &lit, NUMBER, 
            .t_num_lit = &tok->t_num_lit,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
        make_node_expr(state, (C_Ast_ExprLit **)&first, LITERAL,
            .lit = (C_Ast_Literal *)lit,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});

        parser_advance(state);
        break;
    }
    case C_TOKEN_KIND_STRING: {
        C_Ast_LiteralString *lit;
        make_node_lit(state, &lit, STRING, 
            .t_str_lit = &tok->t_str_lit,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
        make_node_expr(state, (C_Ast_ExprLit **)&first, LITERAL,
            .lit = (C_Ast_Literal *)lit,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});

        parser_advance(state);
        break;
    }
    case C_TOKEN_KIND_CHAR: {
        C_Ast_LiteralChar *lit;
        make_node_lit(state, &lit, CHAR, 
            .t_char_lit = &tok->t_char_lit,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});
        make_node_expr(state, (C_Ast_ExprLit **)&first, LITERAL,
            .lit = (C_Ast_Literal *)lit,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1});

        parser_advance(state);
        break;
    }
    case C_TOKEN_KIND_KEYWORD: {
        unimplemented();
        break;
    }
    case C_TOKEN_KIND_EOF: {
        parser_error_expected(state, S("expression"));
        return PARSING_ERROR(NONE);
    }

    
    default: 
        unreacheble();
        break;
    }

    C_Ast_Expr *left = first;



    // postfix
    while (true) {
        auto prev = parser_save(state);
        C_Ast_ExprUnOp *op = nullptr;
        if (IS_ERR(PARSER_OPTIONAL(c_parse_expr_op_postfix_prec_rb(state, &op, max_precedence)))) {
            break;
        }

        if (op->op == C_OPERATOR_FN_CALL) {
            allocator_free(&state->ast_alloc, (void **)&op);
            C_Ast_ExprFnCall *op = nullptr;
            C_Ast_Expr *arg = nullptr;
            darr_T(C_Ast_Expr *) args = nullptr;

            if (IS_OK(PARSER_OPTIONAL(c_parse_expr_assign(state, &arg)))) {
                PARSER_ALLOC_HANDLE(darr_new_cap_in_T(C_Ast_Expr *, 4, &state->ast_alloc, &args));
                PARSER_ALLOC_HANDLE(darr_push(&args, &arg));
                while (true) {
                    if (IS_ERR(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_COMMA, &tok)))) {
                        break;
                    }
                    PARSER_TRY(c_parse_expr_assign(state, &arg));
                    PARSER_ALLOC_HANDLE(darr_push(&args, &arg));
                }
            }
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_PAREN, &tok));
            make_node_expr(state, (C_Ast_ExprFnCall **)&op, FN_CALL,
                .caller = first,
                .args = args,
                .span = parser_span_from_save(state, &prev));
            left = (C_Ast_Expr *)op;
        } else if (op->op == C_OPERATOR_ARRAY_SUB) {
            allocator_free(&state->ast_alloc, (void **)&op);
            C_Ast_ExprArraySub *op = nullptr;
            C_Ast_Expr *arg = nullptr;

            PARSER_OPTIONAL(c_parse_expr(state, &arg));

            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_RIGHT_BRACKET, &tok));
            make_node_expr(state, (C_Ast_ExprArraySub **)&op, ARRAY_SUB,
                .array = first,
                .arg = arg,
                .span = parser_span_from_save(state, &prev));
            left = (C_Ast_Expr *)op;
        } else {
            op->e_operand = left;
            left = (C_Ast_Expr *)op;
        }
    }

    // infix
    // parses at max_precedence level
    while (true) {
        C_Ast_ExprBinOp *op;
        if (IS_ERR(PARSER_OPTIONAL(c_parse_expr_op_infix_prec_rb(state, &op, max_precedence, bitfield_is_flag_set(&flags, C_EXPR_FLAG_STRICT_PRECEDENCE))))) {
            break;
        }

        op->e_lhs = left;
        // parse increasing precedence
        PARSER_TRY(_c_parse_expr(state, &op->e_rhs, c_operator_precedence(op->op), C_EXPR_FLAG_EMPTY));
        left = (C_Ast_Expr *)op;
    }

    // ternary condition operator
    {
        C_Ast_ExprCondOp *op = nullptr;
        if (IS_OK(PARSER_OPTIONAL(c_parse_expr_op_tern_cond_prec_rb(state, &op, max_precedence, bitfield_is_flag_set(&flags, C_EXPR_FLAG_STRICT_PRECEDENCE))))) {
            op->e_cond = left;
            PARSER_TRY(c_parse_expr(state, &op->e_then));
            PARSER_TRY(c_parse_t_punct_kind(state, C_PUNCT_COLON, &tok));
            PARSER_TRY(c_parse_expr_cond(state, &op->e_else));
            left = (C_Ast_Expr *)op;
        }
    }


    bitfield_set_flags(&left->flags, bitfield_mask(&flags, C_EXPR_FLAG_IN_PARENS));
    *out_expr = left;

    PARSING_OK(state);
}

INLINE
ParsingError
c_parse_expr_assign(ParserState *state, C_Ast_Expr **out_expr) {
    return _c_parse_expr(state, out_expr, c_operator_precedence(C_OPERATOR_ASSIGN)+1, C_EXPR_FLAG_STRICT_PRECEDENCE);
}
INLINE
ParsingError
c_parse_expr_cond(ParserState *state, C_Ast_Expr **out_expr) {
    return _c_parse_expr(state, out_expr, c_operator_precedence(C_OPERATOR_TERN_COND)+1, C_EXPR_FLAG_STRICT_PRECEDENCE);
}

ParsingError
c_parse_expr(ParserState *state, C_Ast_Expr **out_expr) {
    C_Token *tok = nullptr;
    C_Ast_Expr *expr = nullptr; 
    auto prev = parser_save(state);

    // won't parse ',' due to strict precedence flag
    TRY(_c_parse_expr(state, &expr, C_MAX_PRECEDENCE, C_EXPR_FLAG_STRICT_PRECEDENCE));
    if (IS_OK(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_COMMA, &tok)))) {
        darr_T(C_Ast_Expr *) items; 
        PARSER_ALLOC_HANDLE(darr_new_cap_in_T(C_Ast_Expr *, 4, &state->ast_alloc, &items));
        PARSER_ALLOC_HANDLE(darr_push(&items, &expr));

        while (true) {
            PARSER_TRY(_c_parse_expr(state, &expr, C_MAX_PRECEDENCE, C_EXPR_FLAG_STRICT_PRECEDENCE));
            PARSER_ALLOC_HANDLE(darr_push(&items, &expr));
            if (IS_ERR(PARSER_OPTIONAL(c_parse_t_punct_kind(state, C_PUNCT_COMMA, &tok)))) {
                break;
            }
        }

        make_node_expr(state, (C_Ast_ExprCompound **)out_expr, COMPOUND,
            .items = items);
    } else {
        *out_expr = expr;
    }

    PARSING_OK(state);
}