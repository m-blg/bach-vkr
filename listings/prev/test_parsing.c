
#include "parsing/c/parsing.c"

#define c_ast_unparse_println(node_kind, node, data) { \
    auto fmt = string_formatter_default(&g_ctx.stdout_sw); \
    ASSERT_OK(c_ast_##node_kind##_unparse_fmt((node), &fmt, data)); \
    ASSERT_OK(string_formatter_write(&fmt, S("\n"))); \
    ASSERT_OK(string_formatter_done(&fmt)); \
}

void
test1() {
    auto ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_ident = (C_Ast_TypeIdent) {
            .ty_kind = C_AST_TYPE_KIND_IDENT,
            .name = S("TypeA"),
        },
    };
    // auto arr2_ty = (C_Ast_Type) {
    //     .kind = C_AST_NODE_KIND_TYPE_NAME,
    //     .ty_array = (C_Ast_TypeArray) {
    //         .ty_kind = C_AST_TYPE_KIND_ARRAY,
    //         .item = &ty,
    //         .count = nullptr,
    //     },
    // };
    auto pointer_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_pointer = (C_Ast_TypePointer) {
            .ty_kind = C_AST_TYPE_KIND_POINTER,
            .pointee = &ty,
        },
        .span = (C_ParserSpan) {
            .b_tok = 0,
            .e_tok = 1,
        },
    };
    auto count_expr = (C_Ast_Expr) {
        .kind = C_AST_NODE_KIND_EXPR,
        .lit = (C_Ast_Literal) {
            .kind = C_AST_NODE_KIND_LITERAL,
            .num = (C_Ast_NumberLiteral) {
                .lit_kind = C_AST_LITERAL_KIND_NUMBER,
                .num_ty = C_NUMBER_TYPE_INT,
                .value_int = 16,
            },
        },
        .span = { 0 },
    };
    auto arr_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_array = (C_Ast_TypeArray) {
            .ty_kind = C_AST_TYPE_KIND_ARRAY,
            .item = &ty,
            .count = nullptr,
        },
    };
    auto arr_p_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_array = (C_Ast_TypeArray) {
            .ty_kind = C_AST_TYPE_KIND_ARRAY,
            .item = &pointer_ty,
            .count = nullptr,
        },
    };
    auto p_arr_p_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_pointer = (C_Ast_TypePointer) {
            .ty_kind = C_AST_TYPE_KIND_POINTER,
            .pointee = &arr_p_ty,
        },
    };
    auto pp_arr_p_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_pointer = (C_Ast_TypePointer) {
            .ty_kind = C_AST_TYPE_KIND_POINTER,
            .pointee = &p_arr_p_ty,
        },
    };
    String s;
    c_ast_unparse_println(type, &ty, nullptr);
    c_ast_unparse_println(type, &pointer_ty, nullptr);
    c_ast_unparse_println(type, &arr_ty, nullptr);
    c_ast_unparse_println(type, &arr_p_ty, nullptr);
    c_ast_unparse_println(type, &p_arr_p_ty, nullptr);
    c_ast_unparse_println(type, &pp_arr_p_ty, nullptr);

    auto arr_arr_p_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_array = (C_Ast_TypeArray) {
            .ty_kind = C_AST_TYPE_KIND_ARRAY,
            .item = &arr_p_ty,
            .count = nullptr,
        },
    };
    str_t var_name = S("var");
    c_ast_unparse_println(type, &arr_arr_p_ty, &var_name);

    auto p_arr_arr_p_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_pointer = (C_Ast_TypePointer) {
            .ty_kind = C_AST_TYPE_KIND_POINTER,
            .pointee = &arr_arr_p_ty,
        },
    };
    auto pp_arr_arr_p_ty = (C_Ast_Type) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_pointer = (C_Ast_TypePointer) {
            .ty_kind = C_AST_TYPE_KIND_POINTER,
            .pointee = &p_arr_arr_p_ty,
        },
    };

    c_ast_unparse_println(type, &pp_arr_arr_p_ty, &var_name);
    // print_fmt(S("%s\n"), string_to_str(&s));
    // string_free(&s);
}


// FmtError
// span_dbg_fmt(C_LexerSpan *span, StringFormatter *fmt, void *_) {
//     TRY(string_formatter_write_fmt(fmt, S(
//         "C_LexerSpan:%+\n"
//             "file_path: %s\n"
//             "b_byte_offset: %lu\n"
//             "b_line: %lu\n"
//             "b_col: %lu\n"
//             "e_byte_offset: %lu\n"
//             "e_line: %lu\n"
//             "e_col: %lu%-"),

//         span->file_path,
          
//         span->b_byte_offset,
//         span->b_line,
//         span->b_col,
          
//         span->e_byte_offset,
//         span->e_line,
//         span->e_col            
//     ));
//     return FMT_ERROR(OK);
// }
FmtError
gen_dbg_fmt_fmt(C_Ast_TypeStruct *st, StringFormatter *fmt, void *_) {
    // String fmt_str;
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
test2() {
    auto field1 = (C_Ast_Decl) {
        .kind = C_AST_NODE_KIND_DECL,
        .ty = &(C_Ast_Type) {
            .kind = C_AST_NODE_KIND_TYPE_NAME,
            .ty_ident = (C_Ast_TypeIdent) {
                .ty_kind = C_AST_TYPE_KIND_IDENT,
                .name = S("TypeA"),
            },
        },
        .name = &(C_Ast_Ident) {
            .kind = C_AST_NODE_KIND_IDENT,
            .name = S("field1")
        }
    };
    auto field2 = (C_Ast_Decl) {
        .kind = C_AST_NODE_KIND_DECL,
        .ty = &(C_Ast_Type) {
            .kind = C_AST_NODE_KIND_TYPE_NAME,
            .ty_ident = (C_Ast_TypeIdent) {
                .ty_kind = C_AST_TYPE_KIND_IDENT,
                .name = S("int"),
            },
        },
        .name = &(C_Ast_Ident) {
            .kind = C_AST_NODE_KIND_IDENT,
            .name = S("field2")
        }
    };
    c_ast_unparse_println(decl, &field1, nullptr);
    c_ast_unparse_println(decl, &field2, nullptr);

    darr_t fields;
    ASSERT_OK(darr_new_cap_in_T(C_Ast_Decl, 3, &g_ctx.global_alloc, &fields));
    darr_push(&fields, &field1);
    darr_push(&fields, &field2);
    auto struct1 = (C_Ast_TypeStruct) {
        .ty_kind = C_AST_TYPE_KIND_STRUCT,
        .name = &(C_Ast_Ident) {
            .kind = C_AST_NODE_KIND_IDENT,
            .name = S("StructA"),
        },
        .fields = fields,
    };
    auto struct_decl = (C_Ast_Decl) {
        .kind = C_AST_NODE_KIND_DECL,
        .ty = &(C_Ast_Type) {
            .kind = C_AST_NODE_KIND_TYPE_NAME,
            .ty_struct = struct1,
        },
        .name = &(C_Ast_Ident) {
            .kind = C_AST_NODE_KIND_IDENT,
            .name = S("field3")
        }
    };

    c_ast_unparse_println(struct, &struct1, nullptr);
    c_ast_unparse_println(decl, &struct_decl, nullptr);
    struct_decl.name = nullptr;
    darr_get_T(C_Ast_Decl, struct_decl.ty->ty_struct.fields, 0)->ty->ty_ident.name = S("int");
    c_ast_unparse_println(decl, &struct_decl, nullptr);

    // gen_dbg_fmt_fmt()
    println(gen_dbg_fmt, &struct1);

    darr_free(&fields);
}
typedef struct StructA StructA;
struct StructA {
    int field1;
    int field2;
};
FmtError
StructA_dbg_fmt(StructA *self, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write_fmt(fmt, S(
        "StructA:%+\n"
            "field1: %d\n"
            "field2: %d\n%-"
        ),
        self->field1,
        self->field2
    ));
    return FMT_ERROR(OK);
}
void
test3() {
    auto a = (StructA) {
        .field1 = 3,
        .field2 = 4,
    };
    dbgp(StructA, &a);
}


// typedef struct D D;
// register struct D {
//     int field1, (*(*field1_2)[3])(int), *field3[5];
//     int field2;
// } g_a;

void
test4() {
    // str_t text = S("\"text\" / \"more text\" \0");
    // str_t text = S("\"text\" , ... : _ident  /** \"more text\" **/ \0");
    str_t text = S(
        "struct A {"
        );
    LexerState state;
    lexer_init_default(&state, text, S("<file>"));
    darr_t tokens;
    ASSERT_OK(tokenize(&state, &tokens));

    // print_tokens(tokens, text);
    // dbgp(c_token, darr_get_T(C_Token, tokens, 0), .data = &text);


    C_Token *tok = nullptr;
    C_Ast_Ident *ident = nullptr;

    ParserState pstate = (ParserState) {
        .tokens = tokens->data,
        .ast_alloc = g_ctx.global_alloc,
    };
    // parser_init_default(&pstate, tokens);

    ASSERT_OK(c_parse_keyword(&pstate, C_KEYWORD_STRUCT, &tok));
    dbgp(c_token, tok, .data = &text);
    ASSERT_OK(c_parse_ident(&pstate, &ident));
    c_ast_unparse_println(ident, ident, nullptr);
    ASSERT_OK(c_parse_punct(&pstate, C_PUNCT_LEFT_BRACE, &tok));
    dbgp(c_token, tok, .data = &text);


    allocator_free(&g_ctx.global_alloc, (void **)&ident);
    // str_t c = darr_get_T(Token, tokens, 0)->content.str;
    // printlnf("%.*s", (int)str_len(c), (char *)c.ptr);
    darr_free(&tokens);
}

void
test6() {
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

    darr_free(&tokens);
}


double;

void
test5() {
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

int
main() {
    ctx_init_default();

    // test1();
    // test2();
    // test3();
    // int field1 = 3, (*(*field1_2)[3])(int), *field1_3[] = {0};
    // test4();
    test5();
}
