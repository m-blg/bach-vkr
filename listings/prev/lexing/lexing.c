#define CORE_IMPL
#include "core/string.h"
#include "core/io.h"
#include "core/array.h"

#include "parsing/c/def.h"




/// (int(CharUTF8), int(CharUTF8)) -> (ParserState -> str_t, ParserState | ParsingError)
/// @param[in, out] state
/// @param[out] out_char
// LexingError
// lex_char_range(LexerState *state, rune_t range_min, rune_t range_max, CharUTF8 *out_char) {
//     // if (lexer_state_len(state) < 1) {
//     //     return LEXING_ERROR(EOF);
//     // }

//     auto rune = char_to_rune(lexer_rest_get(state, 0));
//     if (rune < range_min || range_max < rune) {
//         return LEXING_ERROR(NONE);
//     }
    
//     *out_char = lexer_rest_get(state, 0);
//     lexer_advance(in_out_state, 1);
//     return LEXING_ERROR(OK);
// }

struct_def(ASCII_CharSet, {
    u16_t bits[8];
})

INLINE
bool
ascii_char_set_is_in(uchar_t c, ASCII_CharSet set) {
    return (set.bits[c / 16] & ( 1 << (c % 16) )) > 0;
    
}

#define ASCII_SET_IDENT_NON_DIGIT ((ASCII_CharSet) {\
    .bits[4] = 0b1111'1111'1111'1110,\
    .bits[5] = 0b0000'0111'1111'1111,\
    .bits[6] = 0b1111'1111'1111'1110,\
    .bits[7] = 0b0000'0111'1111'1111,\
})\

#define ASCII_SET_IDENT ((ASCII_CharSet) {\
    .bits[3] = 0b0000'0011'1111'1111,\
    .bits[4] = 0b1111'1111'1111'1110,\
    .bits[5] = 0b0000'0111'1111'1111,\
    .bits[6] = 0b1111'1111'1111'1110,\
    .bits[7] = 0b0000'0111'1111'1111,\
})\

#define ASCII_SET_PUNCT ((ASCII_CharSet) {\
    .bits[2] = 0b0000'0000'0000'0001,\
    .bits[3] = 0b1111'1100'0000'0000,\
    .bits[4] = 0b0000'0000'0000'0001,\
    .bits[5] = 0b1111'1000'0000'0000,\
    .bits[6] = 0b0000'0000'0000'0001,\
    .bits[7] = 0b0111'1000'0000'0000,\
})\

/// @param[in, out] state
/// @param[out] out_char
LexingError
lex_ascii_char_set(LexerState *state, ASCII_CharSet set, uchar_t *out_char) {
    rune_t r = lexer_peek_rune(state);
    if (r > 256) {
        return LEXING_ERROR(NONE);
    }
    uchar_t c = (uchar_t)r;
    if (!ascii_char_set_is_in(c, set)) {
        return LEXING_ERROR(NONE);
    }

    lexer_advance_rune(state);
    *out_char = c;
    return LEXING_ERROR(OK);
}

/// str_t -> (ParserState -> str_t, ParserState | ParseError)
LexingError
lex_string(LexerState *state, str_t lex_str, str_t *out_str) {

    // if (str_len(lex_str) > str_len(lexer_rest(state))) {
    //     return LEXING_ERROR(NONE);
    // }
    // if (!str_eq(lex_str, str_slice(lexer_rest(state), 0, str_len(lex_str)))) {
    //     return LEXING_ERROR(NONE);
    // }
    if (!str_is_prefix(lex_str, lexer_rest(state))) {
        return LEXING_ERROR(NONE);
    }
    *out_str = str_byte_slice(lexer_rest(state), 0, str_len(lex_str));
    for_in_range(_, 0, str_len(lex_str), {
        lexer_advance_rune(state);
    })

    return LEXING_ERROR(OK);
}

#define LEXING_NONE(state, prev) {           \
    lexer_restore(state, prev);             \
    return LEXING_ERROR(NONE);         \
}                                      \

#define LEXING_OK(state) {              \
    return LEXING_ERROR(OK);            \
}                                       \


LexingError
lex_escape_sequence(LexerState *state, rune_t *out_rune) {


    unimplemented();
}

/// @brief lexes string literal token
/// @param[in, out] state 
/// @param[out] out_token 
/// @return 
LexingError
lex_string_literal(LexerState *state, C_Token *out_token) {
    rune_t r = 0;
    auto prev = lexer_save(state);
    r = lexer_advance_rune(state);
    if (r != '"') {
        LEXING_NONE(state, &prev);
    }
    str_t content;// = str_from_ptr_len(lexer_rest(state).ptr, 0);

    while (true) {
        r = lexer_advance_rune(state);
        switch (r)
        {
        case '\0':
            // auto last = lexer_save(state);
            lexer_error(state, S("string literal was not terminated")); 
                // span_from_lexer_savepoints(state, &prev, &last));
            LEXING_NONE(state, &prev);
            break;
        case '\\':
            lex_escape_sequence(state, &r);
            break;
        case '"':
            content = str_from_ptr_len(prev.rest.ptr + 1, (str_len(prev.rest)-1) - (str_len(state->rest) + 1));
            goto end;
            break;
        
        default:
            break;
        }
    }
end:

    auto last = lexer_save(state);
    *out_token = (C_Token) {
        .kind = C_TOKEN_KIND_STRING,

        .span = span_from_lexer_savepoints(state, &prev, &last),
    };
    out_token->t_str_lit.str = content;

    LEXING_OK(state);
}

INLINE
bool
rune_is_space(rune_t r) {
    return (r == ' ' || r == '\t' || r == '\n');
}


INLINE
bool
rune_is_punct(rune_t r) {
    for_in_range(i, 0, slice_len(&g_c_punct_vals), {
        if (r == *str_get_byte(*slice_get_T(str_t, &g_c_punct_vals, i), 0)) {
            return true;
        }
    })
    return false;
    // unimplemented();
}
INLINE
bool
rune_is_ascii_ident_non_digit(rune_t r) {
    if (r > 256) {
        return false;
    }
    return ascii_char_set_is_in((uchar_t)r, ASCII_SET_IDENT_NON_DIGIT);
}
INLINE
bool
rune_is_ascii_punct(rune_t r) {
    if (r > 256) {
        return false;
    }
    return ascii_char_set_is_in((uchar_t)r, ASCII_SET_PUNCT);
}

LexingError
lex_comment(LexerState *state, C_Token *out_token) {
    auto prev = lexer_save(state);
    str_t content;
    if (lex_string(state, S("//"), &content) != LEXING_ERROR(OK)) {
        lexer_error(state, S("Comment was expected here"));
    }
    while (true)
    {
        rune_t r = lexer_peek_rune(state);
        if (r == '\n' || r == '\0') {
            break;
        }
        lexer_advance_rune(state);
    }
    content = (str_t) {
        .ptr = prev.rest.ptr + 2,
        .byte_len = (uintptr_t)lexer_rest(state).ptr - (uintptr_t)prev.rest.ptr - 3,
    };

    *out_token = (C_Token) {
        .kind = C_TOKEN_KIND_COMMENT,
        .span = span_from_lexer_savepoint(state, &prev),
    };
    out_token->t_comment.text = content;
    out_token->t_comment.is_multiline = false;
    LEXING_OK(state);
}
LexingError
lex_multiline_comment(LexerState *state, C_Token *out_token) {
    auto prev = lexer_save(state);
    str_t content;
    if (lex_string(state, S("/*"), &content) != LEXING_ERROR(OK)) {
        lexer_error(state, S("Comment was expected here"));
    }
    while (true)
    {
        rune_t r = lexer_peek_rune(state);
        if (r == '*') {
            auto peek = lexer_save(state);
            lexer_advance_rune(state);
            r = lexer_advance_rune(state);
            if (r == '/') {
                break;
            } else {
                lexer_restore(state, &peek);
            }

        } else if (r == '\0') {
            lexer_error(state, S("Comment was not terminated"));
            LEXING_NONE(state, &prev);
        }
        lexer_advance_rune(state);
    }
    content = (str_t) {
        .ptr = prev.rest.ptr + 2,
        .byte_len = (uintptr_t)lexer_rest(state).ptr - (uintptr_t)prev.rest.ptr - 5,
    };

    *out_token = (C_Token) {
        .kind = C_TOKEN_KIND_COMMENT,
        .span = span_from_lexer_savepoint(state, &prev),
    };
    out_token->t_comment.text = content;
    out_token->t_comment.is_multiline = true;
    LEXING_OK(state);
}


// #define LEXER_TRY(state, expr) {
//     auto _e_ = (expr);          
//     if (IS_ERR(_e_)) {          
//         state->error_handler
//         return _e_;             
//     }                           
//   }                             

/// when sublexer errors out:
/// 1. sublexer prints error message
/// 2. 'tokenize' abort on first error, cause it's probably unreasonable 
///        to continue on broken state here


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
    print(str, &s);   
}



#ifdef DBG_PRINT
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
    str_t _text = str_byte_slice(*(str_t *)text, token->span.b_byte_offset, token->span.e_byte_offset);
    TRY(string_formatter_write_fmt(fmt, S(
        "Token:%+\n"
            "kind: %v,\n"
            "span: %+%v%-,\n"
            "text: %s%-"),
        fmt_obj_pref(token_kind_dbg, &token->kind),
        fmt_obj_pref(span_dbg, &token->span),
        _text
    ));
    return FMT_ERROR(OK);
}


void
dbg_print_tokens(darr_T(C_Token) tokens, str_t text) {
    for_in_range(i, 0, darr_len(tokens), {
        // print_token_by_span(darr_get_T(Token, tokens, i), text);
        dbgp(c_token, darr_get_T(C_Token, tokens, i), &text);
        // println(str, &S(""));
    })
    println(str, &S(""));
}

#endif // DBG_PRINT