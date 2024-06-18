
LexingError
tokenize(LexerState *state, darr_T(Token) *out_tokens) {
    darr_T(Token) tokens;
    darr_new_cap_in_T(C_Token, str_len(state->text)+1, &g_ctx.global_alloc, &tokens);
    C_Token *cur_token = darr_get_unchecked_T(C_Token, tokens, 0);

    while (true) {
        auto prev = lexer_save(state);
        rune_t r = lexer_peek_rune(state);

        switch (r)
        {
        case '\0':
            auto pos = lexer_pos(state);
            *cur_token = (C_Token) {
                .kind = C_TOKEN_KIND_EOF,
                .span = span_from_lexer_pos(&pos, &pos),
            };
            tokens->len += 1;
            goto out;
            break;
        case ' ':
        case '\t':
        case '\n':
            while (true) {
                if (!rune_is_space(lexer_peek_rune(state))) {
                    break;
                }
                lexer_advance_rune(state);
            }
            
            continue;
            break;
        // case '\'':
        //     TRY(lex_character_literal(state, cur_token));
        //     break;
        case '/':
            r = lexer_advance_rune(state);
            r = lexer_peek_rune(state);
            if (r == '/') {
                lexer_restore(state, &prev);
                TRY(lex_comment(state, cur_token));
            } else if (r == '*') {
                lexer_restore(state, &prev);
                TRY(lex_multiline_comment(state, cur_token));
            } else {
                *cur_token = (C_Token) {
                    .kind = C_TOKEN_KIND_PUNCT,
                    .span = span_from_lexer_savepoint(state, &prev),
                };
                cur_token->t_punct.punct_kind = C_PUNCT_SLASH;
            }

            break;
        case '"':
            TRY(lex_string_literal(state, cur_token));
            break;
        
        default:
            if (rune_is_ascii_ident_non_digit(r)) {
                TRY(lex_ident_or_keyword(state, cur_token));
            } else if (rune_is_punct(r)) {
                TRY(lex_punct(state, cur_token));
                break;
            }
            break;
        }
        cur_token += 1;
        tokens->len += 1;
    }
out:

    *out_tokens = tokens;
    return LEXING_ERROR(OK);
}