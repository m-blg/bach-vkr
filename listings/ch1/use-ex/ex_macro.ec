@post_include("parsing/c/proc_macro.h")

FmtError
gen_dbg_fmt_fmt(C_Ast_TypeStruct *st, StringFormatter *fmt, void *_) {
    // String fmt_str;
    string_formatter_write_fmt(fmt, S(
        "FmtError\n"
        "%s_dbg_fmt(%s *self, StringFormatter *fmt, void *_) {\n%+"
            "TRY(string_formatter_write_fmt(fmt, S(%+\n"
            "\"%s:%%+\\n%+\"\n"
            // ")))"
        ),
        st->name->name, st->name->name, st->name->name);

    for_in_range(i, 0, darr_len(st->fields)-1) {
        auto field = darr_get_T(C_Ast_Decl, st->fields, i);
        str_t field_ty_fmt = S("%d");
        string_formatter_write_fmt(fmt, S("\"%s: %s\\n\"\n"), field->d_var.name->name, field_ty_fmt);
    }
    {
        auto field = darr_get_iT(C_Ast_Decl, st->fields, -1);
        str_t field_ty_fmt = S("%d");
        string_formatter_write_fmt(fmt, S("\"%s: %s\\n%%-\"\n"), field->d_var.name->name, field_ty_fmt);
    }
    string_formatter_write_fmt(fmt, S("%-),\n"));

    for_in_range(i, 0, darr_len(st->fields)-1) {
        auto field = darr_get_T(C_Ast_Decl, st->fields, i);
        string_formatter_write_fmt(fmt, S("self->%s,\n"), field->d_var.name->name);
    }
    {
        auto field = darr_get_iT(C_Ast_Decl, st->fields, -1);
        string_formatter_write_fmt(fmt, S("self->%s\n"), field->d_var.name->name);
    }

    string_formatter_write_fmt(fmt, S(
        "%-));\n"
        "return FMT_ERROR(OK);\n"
        ));
    string_formatter_write_fmt(fmt, S("%-}"));
    return FMT_ERROR(OK);
}

@derive_macro(DebugFormat)
ProcMacroError
gen_dbg_fmt(C_TranslationUnitData *data, C_Ast_Decl *decl, C_Ast_TranslationUnit **out_node) {
    if (decl->kind != C_AST_NODE_KIND_DECL || 
        decl->decl_kind != C_AST_DECL_KIND_TYPE_DECL) {
        return PROC_MACRO_ERROR(ERROR);
    }
    auto ty = decl->d_type.ty;
    if (ty->ty_kind != C_AST_TYPE_KIND_STRUCT) {
        return PROC_MACRO_ERROR(ERROR);
    }

    auto st = &ty->ty_struct;

    Allocator string_alloc = arena_allocator(&data->string_arena);
    String s;
    ASSERT_OK(string_new_cap_in(1024, &string_alloc, &s));
    auto fmt = string_formatter_default(&string_sw(&s));
    ASSERT_OK(gen_dbg_fmt_fmt(st, &fmt, nullptr)); 

    C_Ast_TranslationUnit *tr_unit;
    if (IS_ERR(ec_proc_macro_parse_tr_unit(data, string_to_str(&s), S("<gen_dbg_fmt>"), &tr_unit))) {
        return PROC_MACRO_ERROR(ERROR);
    }

    *out_node = tr_unit;

    return PROC_MACRO_ERROR(OK);
}

