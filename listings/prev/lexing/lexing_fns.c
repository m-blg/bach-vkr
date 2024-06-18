
LexingError
lex_punct(LexerState *state, C_Token *out_token) {
    auto prev = lexer_save(state);

    rune_t r = lexer_peek_rune(state);
    usize_t i = 0;
    for (; i < slice_len(&g_c_punct_vals); i += 1) {
        auto punct_lit = slice_get_T(str_t, &g_c_punct_vals, i);
        if (r == *str_get_byte(*punct_lit, 0)) {
            if (str_is_prefix(*punct_lit, lexer_rest(state))) {
                for_in_range(_, 0, str_len(*punct_lit), {
                    lexer_advance_rune(state);
                })
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
    // else if (rune_is_ascii_punct(lexer_peek_rune(state))) {
    //     lexer_error(state, S("Unknown punctuator"));
    //     LEXING_NONE(state, &prev);
    // }

    *out_token = (C_Token) {
        .kind = C_TOKEN_KIND_PUNCT,

        .span = span_from_lexer_savepoint(state, &prev),
    };
    out_token->t_punct.punct_kind = i;

    LEXING_OK(state);
}

LexingError
lex_ident_or_keyword(LexerState *state, C_Token *out_token) {
    auto prev = lexer_save(state);
    str_t content = (str_t) {
        .ptr = lexer_rest(state).ptr,
        .byte_len = 1,
    };
    uchar_t c;
    if (lex_ascii_char_set(state, ASCII_SET_IDENT_NON_DIGIT, &c) != LEXING_ERROR(OK)) {
        return LEXING_ERROR(NONE);
    }
    while (lex_ascii_char_set(state, ASCII_SET_IDENT, &c) == LEXING_ERROR(OK)) {
        content.byte_len += 1;
        // lexer_advance_rune(state);
    }
    
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