FmtError
c_ast_ident_compile_graphviz_fmt(C_Ast_Ident *ident, StringFormatter *fmt, usize_t *node_index, usize_t parent_index) {
    usize_t index = node_ind_new(node_index);
    TRY(string_formatter_write_fmt(fmt, S("_%u [label=\"ident: %s\"];\n"), index, ident->name));
    if (parent_index != 0) {
        TRY(graphviz_write_transition(fmt, parent_index, index));
    }
    return FMT_ERROR(OK);
}

FmtError
c_ast_record_compile_graphviz_fmt(C_Ast_TypeRecord *record, StringFormatter *fmt, usize_t *node_index, usize_t parent_index) {
    if (record->name) {
        TRY(c_ast_ident_compile_graphviz_fmt(record->name, fmt, node_index, parent_index));

    }
    if (record->fields) {
        usize_t fields_index = node_ind_new(node_index);
        TRY(string_formatter_write_fmt(fmt, S("_%u [label=\"fields:\"];\n"), fields_index));
        TRY(graphviz_write_transition(fmt, parent_index, fields_index));

        TRY(string_formatter_write(fmt, S(" ")));
        TRY(string_formatter_write_fmt(fmt, S("subgraph fields_%u {\n"), fields_index));
        string_formatter_pad_inc(fmt);
        for_in_range(i, 0, darr_len(record->fields)) {
            TRY(c_ast_decl_compile_graphviz_fmt(darr_get_T(C_Ast_Decl, record->fields, i), fmt, node_index, fields_index));
            TRY(string_formatter_write_fmt(fmt, S("\n")));
        }
        string_formatter_pad_dec(fmt);
        TRY(string_formatter_write(fmt, S("}")));
    }
    return FMT_ERROR(OK);
}

FmtError
c_ast_type_compile_graphviz_fmt(C_Ast_Type *ty, StringFormatter *fmt, usize_t *node_index, usize_t parent_index) {
    usize_t index = node_ind_new(node_index);
    switch (ty->ty_kind)
    {
    case C_AST_TYPE_KIND_IDENT:
        TRY(string_formatter_write_fmt(fmt, S("_%u [label=\"type\\nkind: %s\\n name: %s\"];\n"), index, S("Ident"), ty->ty_ident.name));
        if (parent_index != 0) {
            TRY(graphviz_write_transition(fmt, parent_index, index));
        }
        break;
    case C_AST_TYPE_KIND_POINTER:
        compile_graphviz_new_node_with_kind(fmt, "type", index, S("Pointer"), parent_index);
        TRY(c_ast_type_compile_graphviz_fmt(ty->ty_pointer.pointee, fmt, node_index, index));
        break;
    case C_AST_TYPE_KIND_ARRAY:
        if (ty->ty_array.count != nullptr) {
            unimplemented();
        }
        
        compile_graphviz_new_node_with_kind(fmt, "type", index, S("Array"), parent_index);
        TRY(c_ast_type_compile_graphviz_fmt(ty->ty_array.item, fmt, node_index, index));
        break;
    case C_AST_TYPE_KIND_FUNCTION:
        if (ty->ty_fn.args) {
            unimplemented();
        }
        compile_graphviz_new_node_with_kind(fmt, "type", index, S("Function"), parent_index);
        TRY(c_ast_type_compile_graphviz_fmt(ty->ty_fn.ret, fmt, node_index, index));
        break;
    case C_AST_TYPE_KIND_STRUCT:
        compile_graphviz_new_node_with_kind(fmt, "type", index, S("Struct"), parent_index);
        TRY(c_ast_record_compile_graphviz_fmt((C_Ast_TypeRecord *)&ty->ty_struct, fmt, node_index, index));
        break;
    case C_AST_TYPE_KIND_UNION:
    case C_AST_TYPE_KIND_ENUM:
        unimplemented();
        break;
    
    default:
        unreacheble();
        break;
    }

    return FMT_ERROR(OK);
}