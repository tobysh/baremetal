#define LONG_MAX 9223372036854775807L
#define LONG_MIN (-9223372036854775807L - 1)
extern long max3(long a, long b, long c);
extern int sign_of(long x);
extern unsigned long sat_sub(unsigned long a, unsigned long b);

static uint64_t seed = 99;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 29); }

int main(void) {
    long r; int s; unsigned long u;
    r = max3(1, 2, 3);    CHECK("max3 ascending", r == 3, "got %ld", r);
    r = max3(3, 2, 1);    CHECK("max3 descending", r == 3, "got %ld", r);
    r = max3(2, 9, 4);    CHECK("max3 middle", r == 9, "got %ld", r);
    r = max3(-5, -2, -9); CHECK("max3 negatives", r == -2, "got %ld", r);
    r = max3(7, 7, 7);    CHECK("max3 ties", r == 7, "got %ld", r);
    r = max3(LONG_MIN, 0, LONG_MIN); CHECK("max3 extremes", r == 0, "got %ld", r);
    int ok = 1; long x = 0;
    for (int i = 0; i < 300 && ok; i++) {
        long a = (long)rnd(), b = (long)rnd(), c = (long)rnd();
        long e = a > b ? a : b; if (c > e) e = c;
        if (max3(a, b, c) != e) { ok = 0; x = a; }
    }
    CHECK("max3 random", ok, "wrong when a = %ld", x);

    s = sign_of(42);        CHECK("sign_of positive", s == 1, "got %d", s);
    s = sign_of(-42);       CHECK("sign_of negative", s == -1, "got %d", s);
    s = sign_of(0);         CHECK("sign_of zero", s == 0, "got %d", s);
    s = sign_of(LONG_MIN);  CHECK("sign_of LONG_MIN", s == -1, "got %d", s);
    s = sign_of(LONG_MAX);  CHECK("sign_of LONG_MAX", s == 1, "got %d", s);
    s = sign_of(1L << 40);  CHECK("sign_of 2^40 (all of rdi, not edi)", s == 1, "got %d", s);

    u = sat_sub(10, 3);     CHECK("sat_sub normal", u == 7, "got %lu", u);
    u = sat_sub(3, 10);     CHECK("sat_sub saturates", u == 0, "got %lu", u);
    u = sat_sub(5, 5);      CHECK("sat_sub equal", u == 0, "got %lu", u);
    u = sat_sub(~0UL, 1);   CHECK("sat_sub treats values as unsigned", u == ~0UL - 1, "got %lu", u);
    u = sat_sub(1, ~0UL);   CHECK("sat_sub 1 - max", u == 0, "got %lu", u);
    ok = 1;
    for (int i = 0; i < 300 && ok; i++) {
        unsigned long a = rnd(), b = rnd();
        if (i & 1) b = a - (rnd() & 0xff);
        unsigned long e = a > b ? a - b : 0;
        if (sat_sub(a, b) != e) { ok = 0; x = (long)a; }
    }
    CHECK("sat_sub random", ok, "wrong when a = %ld", x);
    return 0;
}
