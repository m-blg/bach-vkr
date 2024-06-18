void
main() {
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
    darr_t fields;
    ASSERT_OK(darr_new_cap_in_T(C_Ast_Decl, 3, &g_ctx.global_alloc, &fields));
    darr_push(&fields, &field1);
    darr_push(&fields, &field2);
    auto struct1 = (C_Ast_TypeStruct) {
        .kind = C_AST_NODE_KIND_TYPE_NAME,
        .ty_kind = C_AST_TYPE_KIND_STRUCT,
        .name = &(C_Ast_Ident) {
            .kind = C_AST_NODE_KIND_IDENT,
            .name = S("StructA"),
        },
        .fields = fields,
    };
    auto struct_decl = (C_Ast_Decl) {
        .ty = (C_Ast_Type *)&struct1,
        .name = &(C_Ast_Ident) {
            .kind = C_AST_NODE_KIND_IDENT,
            .name = S("field3")
        }
    };

    c_ast_unparse_println(decl, &struct_decl, nullptr);
    darr_free(&fields);
}