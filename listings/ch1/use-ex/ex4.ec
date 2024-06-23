@post_include("parsing/c/parsing.c")

@derive(DebugFormat)
struct Foo {
    int x;
    int y;
    int z;
};

@derive(DebugFormat)
struct Bar {
    int x;
    int y;
    int z;
};

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