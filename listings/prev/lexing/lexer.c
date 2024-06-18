
enum_def(LexingError, 
    LEXING_ERROR_OK,
    LEXING_ERROR_NONE,
    LEXING_ERROR_EOF,
)
#define LEXING_ERROR(ERR) ((LexingError)LEXING_ERROR_##ERR)

struct_def(Pos, {
    str_t file_path;
    usize_t byte_offset;
    usize_t line;
    usize_t col;
})

struct_def(C_LexerSpan, {
    str_t file_path;

    usize_t b_byte_offset;
    usize_t b_line;
    usize_t b_col;

    usize_t e_byte_offset;
    usize_t e_line;
    usize_t e_col;
})

/// lexing functions assumes, that in passed lexer state the prev state is equal to the current
struct_def(LexerState, {
    str_t text;
    str_t rest; // ptr in text plus rest len
    
    // Meta
    usize_t line;
    usize_t col; 

    str_t file_path;

    // Settings
    void (*utf8_error_handler)(UTF8_Error, Pos pos, str_t, void *);
    void (*error)(str_t, str_t, Pos);
})

void
lexer_utf8_error_handler(UTF8_Error e, Pos pos, str_t note, void *data);
void
lexer_error_print(str_t main_msg, str_t note, Pos pos);

void
lexer_init_default(LexerState *self, str_t text, str_t file_path) {
    *self = (LexerState) {
        .text = text,
        .rest = text,
        .line = 1,
        .col = 1,
        .file_path = file_path,
        .utf8_error_handler = lexer_utf8_error_handler,
        .error = lexer_error_print,
    };
}

struct_def(LexerSavepoint, {
    str_t rest;
    
    usize_t line;
    usize_t col; 
})

INLINE
LexerSavepoint
lexer_save(LexerState *state) {
    return (LexerSavepoint) {
        .rest = state->rest,
        .line = state->line,
        .col = state->col,
    };
}
INLINE
void
lexer_restore(LexerState *state, LexerSavepoint *save) {
    state->rest = save->rest;
    state->line = save->line;
    state->col = save->col;
}

INLINE
Pos
lexer_pos(LexerState *state) {
    return (Pos) {
        .byte_offset = (uintptr_t)state->rest.ptr - (uintptr_t)state->text.ptr,
        .line = state->line,
        .col = state->col,
        .file_path = state->file_path,
    };
}

INLINE
C_LexerSpan
span_from_lexer_savepoint(LexerState *state, 
    LexerSavepoint *prev) 
{
    return (C_LexerSpan) {
        .b_byte_offset = (uintptr_t)prev->rest.ptr - (uintptr_t)state->text.ptr,
        .b_line = prev->line,
        .b_col = prev->col,
        .e_byte_offset = (uintptr_t)state->rest.ptr - (uintptr_t)state->text.ptr,
        .e_line = state->line,
        .e_col = state->col,
    };
}
INLINE
C_LexerSpan
span_from_lexer_savepoints(LexerState *state, 
    LexerSavepoint *begin, LexerSavepoint *end) 
{
    return (C_LexerSpan) {
        .b_byte_offset = (uintptr_t)begin->rest.ptr - (uintptr_t)state->text.ptr,
        .b_line = begin->line,
        .b_col = begin->col,
        .e_byte_offset = (uintptr_t)end->rest.ptr - (uintptr_t)state->text.ptr,
        .e_line = end->line,
        .e_col = end->col,
    };
}
INLINE
C_LexerSpan
span_from_lexer_pos(Pos *begin, Pos *end) 
{
    return (C_LexerSpan) {
        .b_byte_offset = begin->byte_offset,
        .b_line = begin->line,
        .b_col = begin->col,
        .e_byte_offset = end->byte_offset,
        .e_line = end->line,
        .e_col = end->col,
    };
}

#define lexer_rest_len(state) ((state)->rest.rune_len)
#define lexer_rest(state) ((state)->rest)

void
lexer_error(LexerState *state, str_t msg) {
    lexer_error_print(msg, S(""), lexer_pos(state));
}

void
lexer_error_print(str_t main_msg, str_t note, Pos pos) {
    fprintf(stderr, KRED"ERROR: "KNRM"%.*s\n"KNRM"at"KYEL" %.*s:%ld:%ld\n"KNRM, 
        (int)str_len(main_msg), (char *)main_msg.ptr, 
        (int)str_len(pos.file_path), (char *)pos.file_path.ptr, 
        pos.line, pos.col);

    if (str_len(note) > 0) {
        fprintf(stderr, KBLU"NOTE: "KNRM"%.*s\n",
            (int)str_len(note), (char *)note.ptr);
    }
}

void
lexer_utf8_error_handler(UTF8_Error e, Pos pos, str_t note, void *data) {
    switch (e) {
    case UTF8_ERROR(INVALID_RUNE): 
    case UTF8_ERROR(INCOMPLETE_RUNE): 
        lexer_error_print(S("Invalid UTF8 character"), note, pos);
        // fprintf(stderr, "Invalid UTF8 character\nat %.*s:%ld:%ld\n", str_len(pos.file_path), 
        //     pos.file_path.ptr, pos.line, pos.col);
        break;
    case UTF8_ERROR(EMPTY_STRING): 
        lexer_error_print(S("Empty string"), note, pos);
        // fprintf(stderr, "Empty string\nat %.*s:%ld:%ld\n", str_len(pos.file_path), 
        //     pos.file_path.ptr, pos.line, pos.col);
        break;
    default:
        unreachable();
    }
    // fprintf(stderr, "%.*s\n", (int)str_len(msg), msg.ptr);
    exit(1);
}

rune_t
lexer_advance_rune(LexerState *state) {
    if (str_len(state->rest) == 0) {
        return '\0';
    }
    rune_t r;
    auto e = str_next_rune(lexer_rest(state), &r, &lexer_rest(state));
    if (e != UTF8_ERROR(OK)) {
        // char buff[64];
        // usize_t len = snprintf(buff, sizeof(buff), "at %ld:%ld", state->line, state->col);
        // state->utf8_error_handler(e, str_from_ptr_len(buff, len));
        state->utf8_error_handler(e, lexer_pos(state), S(""), nullptr);
        return 0;
    }

    if (r == '\n') {
        state->line += 1;
        state->col = 1;
    } else {
        state->col += 1;
    }

    return r;
}

rune_t
lexer_peek_rune(LexerState *state) {
    if (str_len(state->rest) == 0) {
        return '\0';
    }
    rune_t r;
    str_t s;
    auto e = str_next_rune(lexer_rest(state), &r, &s);
    if (e != UTF8_ERROR(OK)) {
        // char buff[64];
        // usize_t len = snprintf(buff, sizeof(buff), "at %ld:%ld", state->line, state->col);
        // state->utf8_error_handler(e, str_from_ptr_len(buff, len));
        state->utf8_error_handler(e, lexer_pos(state), S(""), nullptr);
        return 0;
        return 0;
    }
    return r;
}