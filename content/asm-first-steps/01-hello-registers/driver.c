#include <limits.h>

extern long add_two(long a, long b);

static void add_case(const char *name, int seeded, long a, long b) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_I("a", a);
    BM_IN_I("b", b);
    BM_OUT_I("add_two(a, b)", add_two(a, b));
}

BM_DRIVER {
    add_case("add_two adds two positive values", 0, 2, 3);
    add_case("add_two handles zero", 0, 0, 0);
    add_case("add_two combines negative and positive values", 0, -4, 10);
    add_case("add_two adds two negative values", 0, -7, -8);
    add_case("add_two preserves values wider than 32 bits", 0, 3000000000L, 4000000000L);
    add_case("add_two preserves the largest signed long", 0, LONG_MAX, 0);
    add_case("add_two preserves the smallest signed long", 0, LONG_MIN, 0);
    add_case("add_two combines opposite extremes", 0, LONG_MIN, LONG_MAX);
    add_case("add_two(a, b) uses both argument registers", 0, 123, 456);
    add_case("add_two(b, a) uses both argument registers", 0, 456, 123);

    static const char *const names[] = {
        "add_two handles fresh values (1)", "add_two handles fresh values (2)",
        "add_two handles fresh values (3)", "add_two handles fresh values (4)",
        "add_two handles fresh values (5)", "add_two handles fresh values (6)",
    };
    for (int i = 0; i < 6; i++) {
        long a = (long)bm_int(-1000000000000L, 1000000000000L);
        long b = (long)bm_int(-1000000000000L, 1000000000000L);
        add_case(names[i], 1, a, b);
    }
}
