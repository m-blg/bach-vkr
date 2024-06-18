LexingError
lex_punct(LexerState *state, C_Token *out_token) {
    auto prev = lexer_save(state);

    rune_t r = lexer_peek_rune(state);
    usize_t i = 0;
    for (; i < slice_len(&g_c_punct_vals); i += 1) {
        auto punct_lit = slice_get_T(str_t, &g_c_punct_vals, i);
        if (r == *str_get_byte(*punct_lit, 0)) {
            if (str_is_prefix(*punct_lit, lexer_rest(state))) {
                for_in_range(_, 0, str_len(*punct_lit)) {
                    lexer_advance_rune(state);
                }
                break;
            }
        }
    }
    if (i == slice_len(&g_c_punct_vals)) {
        LEXING_NONE(state, &prev);
    } 
    else if (i == C_PUNCT_ETC || i == C_PUNCT_DOT) {
        if (lexer_peek_rune(state) == '.') {
            lexer_error(state, S("Unknown punctuator"));
            LEXING_NONE(state, &prev);
        }
    } 
    
    *out_token = (C_Token) {
        .kind = C_TOKEN_KIND_PUNCT,
        .span = span_from_lexer_savepoint(state, &prev),
        .t_punct = (C_TokenPunct) {
            .punct_kind = i,
        },
    };

    LEXING_OK(state);
}

LexingError
lex_ident_or_keyword(LexerState *state, C_Token *out_token) {
    auto prev = lexer_save(state);

    auto string_batch = state->string_batch;
    string_reset(string_batch);
    uchar_t *c = string_end(string_batch);
    rune_t r = 0;

    // first char
    if (IS_ERR(lex_ascii_char_set(state, ASCII_SET_IDENT_NON_DIGIT, c))) {
        if (lexer_peek_rune(state) == '\\') {
            if (IS_ERR(lex_universal_char_name(state, &r))) {
                lexer_error_expected(state, S("universal character name"));
                return LEXING_ERROR(NONE);
            }
            LEXER_ALLOC_HANDLE(string_append_rune(state->string_batch, r));
            c = string_end(string_batch);
        } else {
            return LEXING_ERROR(NONE);
        }
    } else {
        c += 1;
        string_batch->byte_len += 1;
    }

    // rest
    do { 
        LEXER_ALLOC_HANDLE(string_reserve_cap(string_batch, 16));
    for_in_range(_, 0, string_batch->byte_cap)
    {
        if (IS_ERR(lex_ascii_char_set(state, ASCII_SET_IDENT, c))) {

            if (lexer_peek_rune(state) == '\\') {
                if (IS_ERR(lex_universal_char_name(state, &r))) {
                    lexer_error_expected(state, S("universal character name"));
                    return LEXING_ERROR(NONE);
                }
                LEXER_ALLOC_HANDLE(string_append_rune(state->string_batch, r));
                c = string_end(string_batch);
            } else {
                goto out;
            }
        } else {
            c += 1;
            string_batch->byte_len += 1;
            // content.byte_len += 1; // sizeof(uchar_t)
        }
    }} while(1);
    out:

    str_t content;
    lexer_intern_batch(state, &content);

    usize_t i = 0;
    for (; i < slice_len(&g_c_keyword_vals); i += 1) {
        if (str_eq(content, *slice_get_T(str_t, &g_c_keyword_vals, i))) {
            break;
        }
    }
    
    if (i == slice_len(&g_c_keyword_vals)) {
        *out_token = (C_Token) {
            .kind = C_TOKEN_KIND_IDENT,
        };
        out_token->t_ident.name = content;
    } else {
        *out_token = (C_Token) {
            .kind = C_TOKEN_KIND_KEYWORD,
        };
        out_token->t_keyword.keyword_kind = i;
    }
    out_token->span = span_from_lexer_savepoint(state, &prev);
    return LEXING_ERROR(OK);
}