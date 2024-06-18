void
main() {
    str_t text = S(
        "int main() {\n" 
            "bool b;\n" 
            "// bool b;\n" 
            "/* bo*/ol b;\n" 
        "}\n"
        );
    LexerState state;
    lexer_init_default(&state, text, S("<file>"));
    darr_t tokens;
    ASSERT_OK(tokenize(&state, &tokens));

    dbg_print_tokens(tokens, text);

    darr_free(&tokens);
}