FmtError
token_kind_dbg_fmt(C_TokenKind *kind, StringFormatter *fmt) {
#define enum_item_case_fmt_write(item)\
    case item:\
        TRY(string_formatter_write(fmt, S(#item)));\
        break;\

    switch (*kind)
    {
    enum_item_case_fmt_write(C_TOKEN_KIND_INVALID)
    enum_item_case_fmt_write(C_TOKEN_KIND_IDENT)
    enum_item_case_fmt_write(C_TOKEN_KIND_KEYWORD)
    enum_item_case_fmt_write(C_TOKEN_KIND_STRING)
    enum_item_case_fmt_write(C_TOKEN_KIND_CHAR)
    enum_item_case_fmt_write(C_TOKEN_KIND_NUMBER)
    enum_item_case_fmt_write(C_TOKEN_KIND_PUNCT)
    enum_item_case_fmt_write(C_TOKEN_KIND_COMMENT)
    enum_item_case_fmt_write(C_TOKEN_KIND_PP_DIRECTIVE)
    enum_item_case_fmt_write(C_TOKEN_KIND_INCLUDE)
    enum_item_case_fmt_write(C_TOKEN_KIND_EXPAND)
    enum_item_case_fmt_write(C_TOKEN_KIND_NEW_LINE)
    enum_item_case_fmt_write(C_TOKEN_KIND_EOF)
    
    default:
        unreachable();
        break;
    }
    return FMT_ERROR(OK);

#undef enum_item_case_fmt_write
}
// void
// fprint_str(FILE *file, str_t *str) {
//     fprintf(file, "%.*s", (int)str_len(*str), (char *)str->ptr);
//     fwrite();
// }   
// print_str(str_t *str) {
//     fprint_str(stdout, str);
// }
void
print_token_by_span(C_Token *token, str_t text) {
    str_t s = str_byte_slice(text, token->span.b_byte_offset, token->span.e_byte_offset);
    // dbgp(token_kind, &token->kind);   
    auto fmt = string_formatter_default(&g_ctx.stdout_sw);                      \
    ASSERT_OK(token_kind_dbg_fmt(&token->kind, &fmt));
    string_formatter_write(&fmt, S(" "));
    print_pref(str, &s);   
}



FmtError
span_dbg_fmt(C_LexerSpan *span, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write_fmt(fmt, S(
        "C_LexerSpan:%+\n"
            "file_path: %s\n"
            "b_byte_offset: %lu\n"
            "b_line: %lu\n"
            "b_col: %lu\n"
            "e_byte_offset: %lu\n"
            "e_line: %lu\n"
            "e_col: %lu%-"),

        span->file_path,
          
        span->b_byte_offset,
        span->b_line,
        span->b_col,
          
        span->e_byte_offset,
        span->e_line,
        span->e_col            
    ));
    return FMT_ERROR(OK);
}
FmtError
c_token_dbg_fmt(C_Token *token, StringFormatter *fmt, void *text) {
    str_t _text;
    switch (token->kind)
    {
    case C_TOKEN_KIND_IDENT:
        _text = token->t_ident.name;
        break;
    case C_TOKEN_KIND_KEYWORD:
        _text = c_keyword_str_from_kind(token->t_keyword.keyword_kind);
        break;
    case C_TOKEN_KIND_PUNCT:
        _text = c_punct_str_from_kind(token->t_punct.punct_kind);
        break;
    case C_TOKEN_KIND_NUMBER:
        _text = token->t_num_lit.lit;
        break;
    case C_TOKEN_KIND_CHAR:
        _text = token->t_char_lit.char_str;
        break;
    case C_TOKEN_KIND_STRING:
        // ASSERT_OK(imm_print_fmt(S("\"%s\""), token->t_str_lit.str, &_text));
        _text = token->t_str_lit.str;
        break;
    case C_TOKEN_KIND_NEW_LINE:
        _text = S("\\n");
        break;
    
    default:
        // _text = str_byte_slice(*(str_t *)text, 
        //     token->span.b_byte_offset, token->span.e_byte_offset);
        _text = S("unimplemented");
        break;
    }
    TRY(string_formatter_write_fmt(fmt, S(
        "Token:%+\n"
            "kind: %v,\n"
            "flags: %d,\n"
            // "span: %+%v%-,\n"
            "span: %u(%u:%u) - %u(%u:%u) [%s],\n"
            "text: %s%-"),
        fmt_obj_pref(token_kind_dbg, &token->kind),
        (int)token->flags,
        token->span.b_byte_offset, token->span.b_line, token->span.b_col,
        token->span.e_byte_offset, token->span.e_line, token->span.e_col,
        token->span.file_path,
        // fmt_obj_pref(span_dbg, &token->span),
        _text
    ));
    return FMT_ERROR(OK);
}

void
dbg_print_tokens(darr_T(C_Token) tokens, str_t text, 
    hashmap_T(str_t, TU_FileData) file_data_table) 
{
    for_in_range(i, 0, darr_len(tokens)) {
        auto tok = darr_get_T(C_Token, tokens, i);
        if (tok->kind == C_TOKEN_KIND_EXPAND) {
            dbg_print_tokens(tok->t_expand.tokens, text, file_data_table);
        } else if (tok->kind == C_TOKEN_KIND_INCLUDE) {
            str_t text = *hashmap_get_T(str_t, file_data_table, &tok->t_include.file);
            dbg_print_tokens(tok->t_include.tokens, text, file_data_table);
        }
        else {
            dbgp(c_token, darr_get_T(C_Token, tokens, i), &text);
        }
    }
    println_pref(str, &S(""));
}