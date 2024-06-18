LexingError
tokenize(LexerState *state, darr_T(C_Token) *out_tokens) {
    darr_T(C_Token) tokens;
    Allocator token_arena_allocator = arena_allocator(state->token_arena);
    // +1 for EOF
    darr_new_cap_in_T(C_Token, str_len(state->text)+1, &token_arena_allocator, &tokens);
    C_Token *cur_token = darr_get_unchecked_T(C_Token, tokens, 0);

    while (true) {
        // auto prev = lexer_save(state);

        if (IS_ERR(lexer_next_token(state, cur_token))) {
            return LEXING_ERROR(NONE);
        }
        if (cur_token->kind == C_TOKEN_KIND_EOF) {
            tokens->len += 1;
            break;
        }
        if (state->do_preprocessing) {
            if (cur_token->kind == C_TOKEN_KIND_PP_DIRECTIVE) {
                auto dir = &cur_token->t_pp_directive;
                if (dir->pp_dir_kind == C_PP_DIRECTIVE_DEFINE) {
                    darr_t *def = hashmap_get_T(darr_t, state->pp_defs, &dir->ppd_define.name);
                    if (def) {
                        // TODO %s ident
                        lexer_warn(state, S("redefinition of macro"));

                        auto temp = *def;
                        darr_free(&temp);
                        // moved dd_tokens here
                        *def = dir->ppd_define.tokens;
                    } else {
                        LEXER_ALLOC_HANDLE(hashmap_set(&state->pp_defs, &dir->ppd_define.name, &dir->ppd_define.tokens));
                    }
                    continue;
                } else if (dir->pp_dir_kind == C_PP_DIRECTIVE_UNDEF) {
                    darr_t *def = hashmap_get_T(darr_t, state->pp_defs, &dir->ppd_undef.name);
                    if (def) {
                        unimplemented();
                        // hashmap_delete(&state->pp_defs, &dir->ppd_undef.name);
                    }
                } else if (dir->pp_dir_kind == C_PP_DIRECTIVE_INCLUDE) {
                    // unimplemented();

                    str_t file_path = dir->ppd_include.file;

                    str_t include_text;

                    TU_FileData *file_data = hashmap_get(state->file_data_table, &file_path);
                    if (!file_data) {
                        Allocator string_arena_allocator = arena_allocator(state->string_arena);
                        WITH_FILE(file_path, "r", file, {
                            LEXER_ALLOC_HANDLE(file_read_full_str_in(file, &string_arena_allocator, &include_text));
                        })

                        hashmap_set(&state->file_data_table, &file_path, &(TU_FileData) {.text = include_text});
                        file_data = hashmap_get(state->file_data_table, &file_path);
                    } else {
                        include_text = file_data->text;
                    }


                    LexerState sub_lexer = *state;
                    sub_lexer.text = include_text;
                    sub_lexer.rest = include_text;
                    lexer_init_cache(&sub_lexer);

                    darr_t include_tokens;
                    tokenize(&sub_lexer, &include_tokens);
                    *cur_token = (C_Token) {
                        .kind = C_TOKEN_KIND_INCLUDE,
                        .span = cur_token->span,
                        .t_include = (C_TokenInclude) {
                            .file = file_path,
                            .tokens = include_tokens,
                        },
                    };

                } else if (dir->pp_dir_kind == C_PP_DIRECTIVE_IFDEF) {
                    // auto save = lexer_save(state);
                    str_t ident = dir->ppd_ifdef.name;
                    if (hashmap_get(state->pp_defs, &ident) != nullptr) {
                        state->pp_if_depth += 1;
                        continue;
                    } else {
                        goto pp_else;
                    }

                } else if (dir->pp_dir_kind == C_PP_DIRECTIVE_IFNDEF) {
                    // auto save = lexer_save(state);
                    str_t ident = dir->ppd_ifndef.name;

                    if (hashmap_get(state->pp_defs, &ident) == nullptr) {
                        state->pp_if_depth += 1;
                    } else {
                        // seek for #else or #endif
                        pp_else:
                        while(true) {
                            TRY(lexer_next_token(state, cur_token));
                            if (!(cur_token->kind == C_TOKEN_KIND_PUNCT && cur_token->t_punct.punct_kind == C_PUNCT_HASH)) {
                                if (cur_token->kind == C_TOKEN_KIND_EOF) {
                                    break;
                                }
                                continue;
                            }

                            TRY(lexer_next_token(state, cur_token));
                            if (cur_token->kind != C_TOKEN_KIND_IDENT) {
                                lexer_error_expected(state, S("pp directive or identifier"));
                                return LEXING_ERROR(NONE);
                            }
                            str_t directive_name = cur_token->t_ident.name;
                            if (!str_eq(directive_name, S("else")) && !str_eq(directive_name, S("endif"))) {
                                continue;
                            }
                            break;
                        }
                    }

                    continue;
                } else if (dir->pp_dir_kind == C_PP_DIRECTIVE_ELSE) {
                    if (state->pp_if_depth == 0) {
                        lexer_error(state, S("unexpected #else"));
                        return LEXING_ERROR(NONE);
                    }
                    state->pp_if_depth -= 1;

                    // seek for #endif
                    while(true) {
                        TRY(lexer_next_token(state, cur_token));
                        if (!(cur_token->kind == C_TOKEN_KIND_PUNCT && cur_token->t_punct.punct_kind == C_PUNCT_HASH)) {
                            if (cur_token->kind == C_TOKEN_KIND_EOF) {
                                break;
                            }
                            continue;
                        }

                        TRY(lexer_next_token(state, cur_token));
                        if (cur_token->kind != C_TOKEN_KIND_IDENT) {
                            lexer_error_expected(state, S("pp directive or identifier"));
                            return LEXING_ERROR(NONE);
                        }
                        str_t directive_name = cur_token->t_ident.name;
                        if (!str_eq(directive_name, S("endif"))) {
                            continue;
                        }
                        break;
                    }
                    continue;

                } else if (dir->pp_dir_kind == C_PP_DIRECTIVE_ENDIF) {
                    if (state->pp_if_depth == 0) {
                        lexer_error(state, S("unexpected #endif"));
                        return LEXING_ERROR(NONE);
                    }
                    state->pp_if_depth -= 1;
                    continue;
                } else {
                    unimplemented();
                }
            } else if (c_token_is_ident_or_keyword(cur_token)) {
                str_t ident = c_token_ident_or_keyword_name(cur_token);
                darr_t *def = hashmap_get_T(darr_t, state->pp_defs, &ident);
                if (def) {
                    // TODO function-like macros
                    darr_t expanded = nullptr;
                    lexer_pp_expand(state, *def, &expanded);
                    LEXER_ALLOC_HANDLE(darr_reallocate_in(&expanded, &token_arena_allocator));
                    *cur_token = (C_Token) {
                        .kind = C_TOKEN_KIND_EXPAND,
                        .span = cur_token->span,
                        .t_expand = (C_TokenExpand) {
                            .def_name = ident,
                            .tokens = expanded,
                        },
                    };
                }
            } else if (cur_token->kind == C_TOKEN_KIND_STRING) {
                // unimplemented()
                C_Token *first = cur_token;
                auto prev_len = tokens->len;

                // C_Token tok;
                while (true) {
                    cur_token += 1;
                    tokens->len += 1;
                    if (IS_ERR(lexer_next_token(state, cur_token))) {
                        return LEXING_ERROR(NONE);
                    }
                    if (c_token_is_space(cur_token)) {
                        continue;
                    } else if (cur_token->kind != C_TOKEN_KIND_STRING) {
                        break;
                    }

                    // CORRECTNESS: assumes arena is large enough and does allocation in a single chunk
                    first->t_str_lit.str.byte_len += cur_token->t_str_lit.str.byte_len;
                    first->span.e_byte_offset = cur_token->span.e_byte_offset;
                    first->span.e_line = cur_token->span.e_line;
                    first->span.e_col = cur_token->span.e_col;

                    cur_token = first;
                    tokens->len = prev_len;
                }
            }
        }

        cur_token += 1;
        tokens->len += 1;
    }

    *out_tokens = tokens;

    LEXING_OK(state);
}