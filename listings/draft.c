@post_include("demo.h")

@derive(DebugFormat)
struct Foo {
    int x;
    int y;
    int z;
};

int main() {
    ctx_init_default();
    print();
    ctx_deinit();
}

void 
print() {
    Foo foo = (Foo) {
        .x = 3,
        .y = 4,
        .z = 5,
    };
    dbgp(Foo, &foo);
}
%%%
@post_include("demo.h")

struct A {
    B b;
};

int main() {
    ctx_init_default(); 
    g_x = 5;
    foo(g_x);
    ctx_deinit();
}

struct B {
    A *ap;
    C c;
};

int g_x;

void foo(int x) {
    print_fmt(S("x: %d\n"), x);
}


%%%
#include "demo.h"

struct B;
typedef struct B B;
struct A;
typedef struct A A;

struct B {
    A *ap;
    C c;
};
struct A {
    B b;
};

int g_x;

int main();
void foo(int x);

int main() {
    ctx_init_default();
    g_x = 5;
    foo(g_x);
    ctx_deinit();
}
void foo(int x) {
    print_fmt(S("x: %d\n"), x);
}


