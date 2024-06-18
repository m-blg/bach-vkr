
FmtError
c_ast_record_unparse_fmt(C_Ast_TypeRecord *record, StringFormatter *fmt, void *_) {
    if (record->name) {
        TRY(c_ast_ident_unparse_fmt(record->name, fmt, nullptr));
        TRY(string_formatter_write(fmt, S(" ")));
    }
    TRY(string_formatter_write_fmt(fmt, S("{\n")));
    string_formatter_pad_inc(fmt);
    for_in_range(i, 0, darr_len(record->fields), {
        TRY(c_ast_decl_unparse_fmt(darr_get_T(C_Ast_Decl, record->fields, i), fmt, nullptr));
        TRY(string_formatter_write_fmt(fmt, S("\n")));
    })
    string_formatter_pad_dec(fmt);
    TRY(string_formatter_write(fmt, S("}")));
    return FMT_ERROR(OK);
}
FmtError
c_ast_struct_unparse_fmt(C_Ast_TypeStruct *_struct, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write(fmt, S("struct ")));
    TRY(c_ast_record_unparse_fmt((C_Ast_TypeRecord *)_struct, fmt, nullptr))
    return FMT_ERROR(OK);
}
FmtError
c_ast_union_unparse_fmt(C_Ast_TypeUnion *_union, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write(fmt, S("union ")));
    TRY(c_ast_record_unparse_fmt((C_Ast_TypeRecord *)_union, fmt, nullptr))
    return FMT_ERROR(OK);
}

