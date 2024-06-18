void
ec_translation_unit_ast_compile_graphvis(C_TranslationUnitData *self, StreamWriter *dst_sw) {
    auto fmt = string_formatter_default(dst_sw);
    usize_t index = 0;
    ASSERT_OK(ec_ast_translation_unit_compile_graphvis_fmt(self->tr_unit, &fmt, &index, index));
    // ASSERT_OK(string_formatter_write(&fmt, S("\n")));
    ASSERT_OK(string_formatter_done(&fmt));
}

void
test_graphvis() {
    C_TranslationUnitData tr_unit;
    (c_translation_unit_init(&tr_unit, S("sandbox/ex_gv.c")));
    ASSERT(c_translation_unit_lex(&tr_unit));
    // c_translation_unit_dbg_print_tokens(&tr_unit);
    ASSERT(ec_translation_unit_parse(&tr_unit));
    WITH_FILE(S("sandbox/ex_out.dot"), "w", file, {
        OutputFileStream ofs;
        ASSERT_OK(file_sw(file, &ofs));
        auto sw = output_file_stream_stream_writer(&ofs);
        ec_translation_unit_ast_compile_graphvis(&tr_unit, &sw);
    })
}

int
main() {
    ctx_init_default();
    test_graphvis();
    ctx_deinit();
}