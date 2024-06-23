#include "parsing/c/parsing.c"
struct Bar;

typedef struct Bar Bar;

struct Foo;

typedef struct Foo Foo;

struct Bar {
    int x;
    int y;
    int z;
};

struct Foo {
    int x;
    int y;
    int z;
};

FmtError Foo_dbg_fmt(Foo *self, StringFormatter *fmt, void *_);

FmtError Bar_dbg_fmt(Bar *self, StringFormatter *fmt, void *_);

int main();

FmtError Foo_dbg_fmt(Foo *self, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write_fmt(fmt, S("Foo:%+\nx: %d\ny: %d\nz: %d\n%-"), self->x, self->y, self->z));
    return FMT_ERROR(OK);
}

FmtError Bar_dbg_fmt(Bar *self, StringFormatter *fmt, void *_) {
    TRY(string_formatter_write_fmt(fmt, S("Bar:%+\nx: %d\ny: %d\nz: %d\n%-"), self->x, self->y, self->z));
    return FMT_ERROR(OK);
}

int main() {
    
    ctx_init_default();
    Foo foo = (Foo) {
    .x = 3,
    .y = 4,
    .z = 5,
    };
    dbgp(Foo, &foo);
    Bar bar = (Bar) {
    .x = 6,
    .y = 7,
    .z = 8,
    };
    dbgp(Bar, &bar);
    ctx_deinit();
}

