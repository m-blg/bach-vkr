ParsingError
c_parse_translation_unit(ParserState *state, C_Ast_TranslationUnit **out_tr_unit) {
    auto prev = parser_save(state);


    darr_t decls;
    // works with arena reallocations
    PARSER_ALLOC_HANDLE(darr_new_cap_in_T(C_Ast_Decl *, 16, &state->ast_alloc, &decls));

    C_SymbolTable global_scope;
    PARSER_ALLOC_HANDLE(hashmap_new_cap_in_T(C_Symbol, C_SymbolData, 8, &state->ast_alloc, &global_scope));
    c_environment_push_scope(&state->env, &global_scope);

    // c_scope_populate_builtins(&global_scope);

    C_Ast_Decl *decl;
    while (parser_peek(state)->kind != C_TOKEN_KIND_EOF) {
        PARSER_ALLOC_HANDLE(darr_reserve_cap(&decls, 1));
        if (IS_ERR(c_parse_declaration(state, &decl))) {
            PARSING_NONE(state, &prev);
        }
        if (state->collect_symbols) {
            if (!c_parser_scope_collect_symbols_decl(state, &global_scope, decl)) {
                PARSING_NONE(state, &prev);
            }
        }

        *darr_get_unchecked_T(C_Ast_Decl *, decls, darr_len(decls)) = decl;
        decls->len += 1;
        arena_free(state->ast_arena, (void **)&decl);
    }

    c_environment_pop_scope(&state->env);

    make_node(state, out_tr_unit, TR_UNIT, 
            .decls = decls,
            .span = parser_span_from_save(state, &prev),
        );

    return PARSING_ERROR(OK);
}