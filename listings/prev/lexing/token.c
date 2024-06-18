
enum_def(C_TokenKind, 
    C_TOKEN_KIND_INVALID,
    C_TOKEN_KIND_IDENT,
    C_TOKEN_KIND_KEYWORD,
    C_TOKEN_KIND_STRING,
    C_TOKEN_KIND_CHAR,
    C_TOKEN_KIND_NUMBER,
    C_TOKEN_KIND_PUNCT,
    C_TOKEN_KIND_COMMENT,
    C_TOKEN_KIND_EOF,
)

struct_def(C_TokenIdent, {
    str_t name;
})
struct_def(C_TokenKeyword, {
    C_KeywordKind keyword_kind;
})
struct_def(C_TokenPunct, {
    C_PunctKind punct_kind;
})
struct_def(C_TokenComment, {
    str_t text;
    bool is_multiline;
})
struct_def(C_TokenStringLiteral, {
    str_t str;
})
struct_def(C_TokenCharLiteral, {
    rune_t rune;
})
struct_def(C_TokenNumLiteral, {
    union {
        usize_t uval;
        isize_t ival;
    };
})

struct_def(C_Token, {
    C_TokenKind kind;
    union {
        C_TokenIdent t_ident;
        C_TokenKeyword t_keyword;
        C_TokenPunct t_punct;
        C_TokenComment t_comment;
        C_TokenStringLiteral t_str_lit;
        C_TokenCharLiteral t_char_lit;
        C_TokenNumLiteral t_num_lit;
    };

    C_LexerSpan span;
})