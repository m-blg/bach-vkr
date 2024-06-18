#include "parsing/c/lexing.c"
// #define C_AST_NODE_KINDS
// 	C_AST_NODE_KIND(Ident, "identifier", {

// 	})


struct_def(C_ParserSpan, {
    usize_t b_tok;
    usize_t e_tok;
})

/// better go for ast directly, no need for syntax tree

/// Node have dynamic size, depending on its type


/// Ideas on general, extendable data structure, so that
/// User can define its own syntactic elements

/// Type system of syntactic elements

/// The problem basically is how to store code as data, and manupulate it
/// Code block - sequence of statements


// bool
// c_token_is_constant(Token *token) {
//     return  token->kind == TOKEN_KIND_STRING ||
//             token->kind == TOKEN_KIND_CHAR ||
//             token->kind == TOKEN_KIND_NUMBER;
// }
// bool
// c_ast_node_is_literal(C_Ast_Node *node) {
//     return C_AST_NODE_KIND_LITERAL_BEGIN < node->kind &&
//         node->kind < C_AST_NODE_KIND_LITERAL_END;
// }
FmtError
c_ast_ident_unparse_fmt(C_Ast_Ident *ident, StringFormatter *fmt, void *_);
FmtError
c_ast_expr_unparse_fmt(C_Ast_Expr *expr, StringFormatter *fmt, void *_);
FmtError
c_ast_decl_unparse_fmt(C_Ast_Decl *decl, StringFormatter *fmt, void *_);
FmtError
c_ast_record_unparse_fmt(C_Ast_TypeRecord *record, StringFormatter *fmt, void *_);
FmtError
c_ast_struct_unparse_fmt(C_Ast_TypeStruct *_struct, StringFormatter *fmt, void *_);
FmtError
c_ast_union_unparse_fmt(C_Ast_TypeUnion *_union, StringFormatter *fmt, void *_);
FmtError
c_ast_type_unparse_fmt(C_Ast_Type *ty, StringFormatter *fmt, void *var_name);

FmtError
c_ast_ident_unparse_fmt(C_Ast_Ident *ident, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write(fmt, ident->name));
    return FMT_ERROR(OK);
}

FmtError
c_ast_expr_unparse_fmt(C_Ast_Expr *expr, StringFormatter *fmt, void *_) {
    unimplemented();
    return FMT_ERROR(OK);
}

FmtError
c_ast_decl_unparse_fmt(C_Ast_Decl *decl, StringFormatter *fmt, void *_) {
    str_t *name = decl->name ? &decl->name->name : nullptr;
    TRY(c_ast_type_unparse_fmt(decl->ty, fmt, name));
    if (decl->initializer) {
        unimplemented();
    }

    if (decl->others) {
        ASSERT(decl->name);
        unimplemented();
    }
    TRY(string_formatter_write(fmt, S(";")));
    return FMT_ERROR(OK);
}

// FmtError
// c_ast_node_dbg_fmt(C_Ast_Node *node, StringFormatter *fmt, void *text) {
//     switch (node->kind)
//     {
//     case C_AST_NODE_KIND_IDENT:
        
//         break;
    
//     default:
//         unreacheble();
//         break;
//     }
//     str_t _text = str_byte_slice(*(str_t *)text, token->span.b_byte_offset, token->span.e_byte_offset);
//     TRY(string_formatter_write_fmt(fmt, S(
//         "Token:%+\n"
//             "kind: %v,\n"
//             "span: %+%v%-,\n"
//             "text: %s%-"),
//         fmt_obj_pref(token_kind_dbg, &token->kind),
//         fmt_obj_pref(span_dbg, &token->span),
//         _text
//     ));
//     return FMT_ERROR(OK);
// }

// FmtError
// c_ast_if_stmt_unparse_fmt(C_Ast_IfStmt *stmt, StringFormatter *fmt, void *_) {
//     unimplemented();
//     // return 
// }

/// generally
/// parsing procedure per production rule in the C spec
/// ast node per section in C spec

// type - label
// type_data - can be aquired via a hash table

// enum_def(C_TypeKind, 
//     C_TYPE_KIND_PRIMITIVE,
//     C_TYPE_KIND_POINTER,
//     C_TYPE_KIND_ARRAY,
//     C_TYPE_KIND_FUNCTION,
//     C_TYPE_KIND_STRUCT,
//     C_TYPE_KIND_UNION,
//     C_TYPE_KIND_ENUM
// )

// hash_map_T(str_t, C_Type) c_type_table;


struct_def(ParserState, {
    slice_T(C_Token) tokens; 
    usize_t cur;

    Allocator ast_alloc;
})

enum_def(ParsingError, 
    PARSING_ERROR_OK,
    PARSING_ERROR_NONE,
    PARSING_ERROR_EOF,
)
#define PARSING_ERROR(ERR) ((ParsingError)PARSING_ERROR_##ERR)

#define BOUNDS_ASSERT(x) ASSERT(x)

// INLINE
// void
// parser_init_default(ParserState *state, darr_T(C_Token) tokens) {
//     state
// }

INLINE
C_Token *
parser_peek(ParserState *state) {
    BOUNDS_ASSERT(0 <= state->cur && state->cur < slice_len(&state->tokens));
    return slice_get_T(C_Token, &state->tokens, state->cur);
}

INLINE
C_Token *
parser_advance(ParserState *state) {
    BOUNDS_ASSERT(0 <= state->cur && state->cur < slice_len(&state->tokens));
    auto tok = slice_get_T(C_Token, &state->tokens, state->cur);
    if (state->cur < slice_len(&state->tokens)-1) {
        state->cur += 1;
    }
    return tok;
}

struct_def(ParserSavepoint, {
    usize_t cur;
})
INLINE
ParserSavepoint
parser_save(ParserState *state) {
    return (ParserSavepoint) {
        .cur = state->cur,
    };
}
INLINE
void
parser_restore(ParserState *state, ParserSavepoint *save) {
    state->cur = save->cur;
}

#define PARSING_NONE(state, prev) {\
    parser_restore(state, prev);\
    return PARSING_ERROR(NONE);\
}\

#define PARSING_OK() {\
    return PARSING_ERROR(OK);\
}\

void
parser_error(ParserState *state, str_t msg) {
    auto span = &slice_get_T(C_Token, &state->tokens, state->cur)->span;
    auto pos = (Pos) {
        .byte_offset = span->b_byte_offset,
        .line = span->b_line,
        .col = span->b_col,
        .file_path = span->file_path,
    };
    lexer_error_print(msg, S(""), pos);
}

C_ParserSpan
parser_span_from_save(ParserState *state, ParserSavepoint *save) {
    return (C_ParserSpan) {
        .b_tok = save->cur,
        .e_tok = state->cur,
    };
}


#define make_node(state, out_node, KIND_SUFF, args...) {\
    ASSERT_OK(allocator_alloc_T(&state->ast_alloc, typeof(**out_node), out_node));\
    **out_node = ((typeof(**out_node)) {.kind = C_AST_NODE_KIND_##KIND_SUFF, ##args });\
}\

#define make_node_type(state, out_node, TYPE_KIND_SUFF, args...) {\
    ASSERT_OK(allocator_alloc_T(&state->ast_alloc, typeof(**out_node), out_node));\
    **out_node = ((typeof(**out_node)) {\
        .kind = C_AST_NODE_KIND_TYPE_NAME, \
        .ty_kind = C_AST_TYPE_KIND_##TYPE_KIND_SUFF,\
        ##args \
        });\
}\

ParsingError
c_parse_ident(ParserState *state, C_Ast_Ident **out_ident);
ParsingError
c_parse_keyword(ParserState *state, C_KeywordKind kind, C_Token **out_tok);
ParsingError
c_parse_punct(ParserState *state, C_PunctKind kind, C_Token **out_tok);
ParsingError
c_parse_declaration(ParserState *state, C_Ast_Decl **out_decl);

// ParsingError
// c_parse_struct(ParserState *state, C_Ast_TypeStruct **out_st, void *_) {
//     unimplemented();


//     TRY(c_parse_keyword(state, C_KEYWORD_STRUCT, nullptr));

//     C_Ast_TypeRecord *record;
//     c_parse_record(state, &record, nullptr);
//     record->ty_kind = C_AST_TYPE_KIND_STRUCT;
//     *out_st = record;

//     return PARSING_ERROR(OK);
// }

ParsingError
c_parse_ident(ParserState *state, C_Ast_Ident **out_ident) {
    C_Token *tok = parser_peek(state);
    if (tok->kind != C_TOKEN_KIND_IDENT) {
        return PARSING_ERROR(NONE);
    }

    make_node(state, out_ident, IDENT, 
            .name = tok->t_ident.name,
            .span = (C_ParserSpan) {.b_tok = state->cur, .e_tok = state->cur + 1}
        );
    parser_advance(state);
    return PARSING_ERROR(OK);
}

ParsingError
c_parse_keyword(ParserState *state, C_KeywordKind kind, C_Token **out_tok) {
    C_Token *tok = parser_peek(state);
    if (tok->kind != C_TOKEN_KIND_KEYWORD || tok->t_keyword.keyword_kind != kind) {
        return PARSING_ERROR(NONE);
    }

    *out_tok = parser_advance(state);
    return PARSING_ERROR(OK);
}
ParsingError
c_parse_punct(ParserState *state, C_PunctKind kind, C_Token **out_tok) {
    C_Token *tok = parser_peek(state);
    if (tok->kind != C_TOKEN_KIND_PUNCT || tok->t_punct.punct_kind != kind) {
        return PARSING_ERROR(NONE);
    }

    *out_tok = parser_advance(state);
    return PARSING_ERROR(OK);
}

/// @param[in, out] state
/// @param[out] decl_ty_head
/// @param[out] decl_ty_leaf
/// @param[out] decl_name

// ParsingError
// c_parse_translation_unit(ParserState *state, C_Ast_TranslationUnit *out_tu, void *_) {
//     // PARSING_MANY()
//     darr_t decls;
//     ASSERT_OK(darr_new_cap_in_T(C_Ast_Decl, 16, &state->ast_alloc, &decls));

//     C_Ast_Decl decl;
//     while (IS_OK(c_parse_decl(state, &decl, nullptr))) {
//         darr_push(&decls, &decl);
//     }

//     return PARSING_ERROR(OK);
// }
// IDEA: recording format strings to later proper unparse

FmtError
c_ast_type_unparse_fmt(C_Ast_Type *ty, StringFormatter *fmt, void *var_name) {
    String s, temp;
    str_t temp_str;
    C_Ast_TypeKind *prev_ty = nullptr;
    C_Ast_TypeKind type_kind_ident = C_AST_TYPE_KIND_IDENT;
    StringFormatter temp_fmt;
    
    // NOTE: can customize error hander in ASSERT
    ASSERT_OK(string_new_cap_in(64, &g_ctx.global_alloc, &s));
    ASSERT_OK(string_new_cap_in(64, &g_ctx.global_alloc, &temp));
    string_formatter_init_string_default(&temp_fmt, &temp);

    if (var_name) {
        string_append_str(&s, *(str_t *)var_name);
        prev_ty = &type_kind_ident;
    }

    while (true) {
        switch (ty->ty_kind)
        {
        case C_AST_TYPE_KIND_IDENT:
            if (prev_ty == nullptr) {
                string_append_str(&s, ty->ty_ident.name);
            } else {
                string_prepend_str(&s, S(" "));
                string_prepend_str(&s, ty->ty_ident.name);
            }
            goto out;
            break;
        case C_AST_TYPE_KIND_POINTER:
            string_prepend_str(&s, S("*"));
            prev_ty = &ty->ty_kind;
            ty = ty->ty_pointer.pointee;
            continue;
            break;
        case C_AST_TYPE_KIND_ARRAY:
            if (ty->ty_array.count != nullptr) {
                // ASSERT_OK(string_new_cap_in(64, &g_ctx.global_alloc, &temp));
                string_reset(&temp);
                TRY(c_ast_expr_unparse_fmt(ty->ty_array.count, &temp_fmt, nullptr));
                temp_str = string_to_str(&temp);
            } else {
                temp_str = S("");
            }

            if (prev_ty == nullptr || *prev_ty == C_AST_TYPE_KIND_ARRAY 
                                   || *prev_ty == C_AST_TYPE_KIND_IDENT) 
            {
                string_append_str(&s, S("["));
                string_append_str(&s, temp_str);
                string_append_str(&s, S("]"));
            } else {
                string_prepend_str(&s, S("("));
                // string_append_str_fmt(&s, S(")[%s]"), string_to_str(&temp));
                string_append_str(&s, S(")["));
                string_append_str(&s, temp_str);
                string_append_str(&s, S("]"));
            }

            // if (ty->ty_array.count != nullptr) {
            //     string_free(&temp);
            // }
            
            prev_ty = &ty->ty_kind;
            ty = ty->ty_array.item;
            break;
        case C_AST_TYPE_KIND_STRUCT:
            string_reset(&temp);
            TRY(c_ast_struct_unparse_fmt(&ty->ty_struct, &temp_fmt, nullptr));
            if (prev_ty != nullptr) {
                string_append_str(&temp, S(" "));
                string_prepend_str(&s, string_to_str(&temp));
            } else {
                string_append_str(&s, string_to_str(&temp));
            }
            goto out;
            break;
        case C_AST_TYPE_KIND_FUNCTION:
        case C_AST_TYPE_KIND_UNION:
        case C_AST_TYPE_KIND_ENUM:
            unimplemented();
            break;
        // case C_AST_TYPE_KIND_POINTER:
        //     TRY(c_ast_type_unparse_fmt(ty->pointer.pointee, *fmt, nullptr));
        //     break;
        
        default:
            unreacheble();
            break;
        }
    }
out:

    TRY(string_formatter_write(fmt, string_to_str(&s)));
    string_free(&s);
    string_free(&temp);

    return FMT_ERROR(OK);
}