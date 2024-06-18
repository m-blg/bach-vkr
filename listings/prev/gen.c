
#include "parsing/c/parsing.c"

#define c_ast_unparse_println(node_kind, node, data) { \
    auto fmt = string_formatter_default(&g_ctx.stdout_sw); \
    ASSERT_OK(c_ast_##node_kind##_unparse_fmt((node), &fmt, data)); \
    ASSERT_OK(string_formatter_write(&fmt, S("\n"))); \
    ASSERT_OK(string_formatter_done(&fmt)); \
}

FmtError
gen_dbg_fmt_fmt(C_Ast_TypeStruct *st, StringFormatter *fmt, void *_) {
    string_formatter_write_fmt(fmt, S(
        "FmtError\n"
        "%s_dbg_fmt(%s *self, StringFormatter *fmt, void *_) {\n%+"
            "TRY(string_formatter_write_fmt(fmt, S(%+\n"
            "\"%s:\\%+\\\\n%+\"\n"
            // ")))"
        ),
        st->name->name, st->name->name, st->name->name);

    for_in_range(i, 0, darr_len(st->fields)-1, {
        auto field = darr_get_T(C_Ast_Decl, st->fields, i);
        str_t field_ty_fmt = S("%d");
        string_formatter_write_fmt(fmt, S("\"%s: %s\\\\n\"\n"), field->name->name, field_ty_fmt);
    })
    {
        auto field = darr_get_iT(C_Ast_Decl, st->fields, -1);
        str_t field_ty_fmt = S("%d");
        string_formatter_write_fmt(fmt, S("\"%s: %s\\\\n\\%-\"\n"), field->name->name, field_ty_fmt);
    }
    string_formatter_write_fmt(fmt, S("%-),\n"));

    for_in_range(i, 0, darr_len(st->fields)-1, {
        auto field = darr_get_T(C_Ast_Decl, st->fields, i);
        string_formatter_write_fmt(fmt, S("self->%s,\n"), field->name->name);
    })
    {
        auto field = darr_get_iT(C_Ast_Decl, st->fields, -1);
        string_formatter_write_fmt(fmt, S("self->%s\n"), field->name->name);
    }

    string_formatter_write_fmt(fmt, S(
        "%-));\n"
        "return FMT_ERROR(OK);\n"
        ));
    string_formatter_write_fmt(fmt, S("%-}"));
    return FMT_ERROR(OK);
}

void
main() {
    ctx_init_default();

    str_t text = S(
        "struct A {\n"
            "int x;\n"
            "int x2;\n"
            "int x3;\n"
        "};\n"
        );
    LexerState state;
    lexer_init_default(&state, text, S("<file>"));
    darr_t tokens;
    ASSERT_OK(tokenize(&state, &tokens));

    C_Token *tok = nullptr;
    C_Ast_Decl *decl = nullptr;
    C_Ast_Type *ty = nullptr;

    auto pstate = (ParserState) {
        .tokens = tokens->data,
        .ast_alloc = g_ctx.global_alloc,
    };
    pstate = (ParserState) {
            .tokens = tokens->data,
            .ast_alloc = g_ctx.global_alloc,
        };
    ASSERT_OK(c_parse_declaration(&pstate, &decl));
    c_ast_unparse_println(decl, decl, nullptr);


    println(gen_dbg_fmt, (C_Ast_TypeStruct *)decl->ty);
    darr_free(&tokens);
}