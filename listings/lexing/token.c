enum_def(C_TokenKind, 
    C_TOKEN_KIND_INVALID,
    C_TOKEN_KIND_IDENT,
    C_TOKEN_KIND_KEYWORD,
    C_TOKEN_KIND_STRING,
    C_TOKEN_KIND_CHAR,
    C_TOKEN_KIND_NUMBER,
    C_TOKEN_KIND_PUNCT,
    C_TOKEN_KIND_COMMENT,
    C_TOKEN_KIND_PP_DIRECTIVE,
    // C_TOKEN_KIND_HEADER_NAME,
    C_TOKEN_KIND_INCLUDE,
    C_TOKEN_KIND_EXPAND,
    C_TOKEN_KIND_NEW_LINE,
    C_TOKEN_KIND_EOF
)

struct_def(Pos, {
    str_t file_path;
    usize_t byte_offset;
    usize_t line;
    usize_t col;
})

struct_def(C_LexerSpan, {
    str_t file_path;

    usize_t b_byte_offset;
    usize_t b_line;
    usize_t b_col;

    usize_t e_byte_offset;
    usize_t e_line;
    usize_t e_col;
})

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
    str_t char_str;
    // rune_t rune;
})
struct_def(C_TokenNumLiteral, {
    str_t lit; // in case of float may contain '.'
    u8_t base; // 2, 8, 10, 16
    C_PrimitiveType type; // u, ul, ll, ...
})
/// appears in place of #include directive
struct_def(C_TokenInclude, {
    str_t file;
    darr_T(C_Token) tokens;
})
/// appears in place of macro expansion
struct_def(C_TokenExpand, {
    str_t def_name;
    darr_T(C_Token) tokens;
})


enum_def(C_PP_DirectiveKind,
    C_PP_DIRECTIVE_INVALID,
    C_PP_DIRECTIVE_INCLUDE,
    C_PP_DIRECTIVE_DEFINE,
    C_PP_DIRECTIVE_UNDEF,
    C_PP_DIRECTIVE_LINE,
    C_PP_DIRECTIVE_ERROR,
    C_PP_DIRECTIVE_PRAGMA,
    C_PP_DIRECTIVE_IF,
    C_PP_DIRECTIVE_IFDEF,
    C_PP_DIRECTIVE_IFNDEF,
    C_PP_DIRECTIVE_ELIF,
    C_PP_DIRECTIVE_ELSE,
    C_PP_DIRECTIVE_ENDIF
)

struct_def(C_Token_PPD_Define, {
    str_t name;
    darr_T(C_Token) tokens;
})
struct_def(C_Token_PPD_Undef, {
    str_t name;
})
struct_def(C_Token_PPD_Ifdef, {
    str_t name;
})
struct_def(C_Token_PPD_Ifndef, {
    str_t name;
})
struct_def(C_Token_PPD_Include, {
    str_t file;
    uchar_t brackets; // '<', '"' or '\0'(ident)
})

struct_def(C_Token_PP_Directive, {
    C_PP_DirectiveKind pp_dir_kind;
    union {
        C_Token_PPD_Define ppd_define;
        C_Token_PPD_Undef ppd_undef;
        C_Token_PPD_Ifdef ppd_ifdef;
        C_Token_PPD_Ifndef ppd_ifndef;
        C_Token_PPD_Include ppd_include;
    };
})

struct_def(C_TokenHeaderName, {
    str_t name;
    uchar_t brackets; // '<' or '"'
})

typedef u8_t C_TokenFlags; 

typedef enum C_TokenFlag C_TokenFlag; 
enum C_TokenFlag {
    C_TOKEN_FLAG_EMPTY = (u8_t)0,
    C_TOKEN_FLAG_WAS_SPACE = (u8_t)1 << 0,
    C_TOKEN_FLAG_WAS_NEW_LINE = (u8_t)1 << 1,
};

struct_def(C_Token, {
    C_TokenKind kind;
    C_LexerSpan span;
    C_TokenFlags flags;
    union {
        C_TokenIdent t_ident;
        C_TokenKeyword t_keyword;
        C_TokenPunct t_punct;
        C_TokenComment t_comment;
        C_TokenStringLiteral t_str_lit;
        C_TokenCharLiteral t_char_lit;
        C_TokenNumLiteral t_num_lit;
        C_Token_PP_Directive t_pp_directive;
        C_TokenHeaderName t_header_name;
        C_TokenInclude t_include;
        C_TokenExpand t_expand;
    };
})