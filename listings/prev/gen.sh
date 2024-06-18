
struct A {
    int x;
    int x2;
    int x3;
};
FmtError
A_dbg_fmt(A *self, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write_fmt(fmt, S(
        "A:%+\n"
            "x: %d\n"
            "x2: %d\n"
            "x3: %d\n%-"
        ),
        self->x,
        self->x2,
        self->x3
    ));
    return FMT_ERROR(OK);
}