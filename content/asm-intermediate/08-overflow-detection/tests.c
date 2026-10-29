extern int add_checked(long a, long b, long *out);
extern int mul_checked_u(uint64_t a, uint64_t b, uint64_t *out);
extern int fact_checked(unsigned n, uint64_t *out);

static uint64_t seed = 8675309;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 29); }

int main(void) {
    const long MAXL = 9223372036854775807L, MINL = -MAXL - 1;
    long o = 12345; int r;
    r = add_checked(2, 3, &o);       CHECK("2 + 3", r == 1 && o == 5, "returned %d, *out = %ld", r, o);
    r = add_checked(-10, 4, &o);     CHECK("negative operands", r == 1 && o == -6, "returned %d, *out = %ld", r, o);
    o = 777; r = add_checked(MAXL, 1, &o);  CHECK("positive overflow detected", r == 0 && o == 777, "returned %d, *out = %ld", r, o);
    o = 777; r = add_checked(MINL, -1, &o); CHECK("negative overflow detected", r == 0 && o == 777, "returned %d, *out = %ld", r, o);
    r = add_checked(MAXL, MINL, &o); CHECK("max + min does not overflow", r == 1 && o == -1, "returned %d, *out = %ld", r, o);
    r = add_checked(MAXL - 5, 5, &o);CHECK("exactly the maximum", r == 1 && o == MAXL, "returned %d, *out = %ld", r, o);
    int ok = 1;
    for (int i = 0; i < 400 && ok; i++) {
        long a = (long)rnd() >> (rnd() % 3), b = (long)rnd() >> (rnd() % 3), e, g = 4242; o = g;
        int ovf = __builtin_add_overflow(a, b, &e);
        r = add_checked(a, b, &o);
        if (ovf ? (r != 0 || o != g) : (r != 1 || o != e)) ok = 0;
    }
    CHECK("add_checked random", ok, "a random pair was classified wrongly");

    uint64_t u = 99;
    r = mul_checked_u(6, 7, &u);            CHECK("6 * 7", r == 1 && u == 42, "returned %d, *out = %lu", r, u);
    r = mul_checked_u(0, ~0UL, &u);         CHECK("0 * max", r == 1 && u == 0, "returned %d, *out = %lu", r, u);
    u = 99; r = mul_checked_u(1UL << 32, 1UL << 32, &u); CHECK("2^32 * 2^32 overflows", r == 0 && u == 99, "returned %d, *out = %lu", r, u);
    r = mul_checked_u(1UL << 32, (1UL << 32) - 1, &u);   CHECK("largest product that fits", r == 1 && u == (1UL << 32) * ((1UL << 32) - 1), "returned %d, *out = %lu", r, u);
    ok = 1;
    for (int i = 0; i < 400 && ok; i++) {
        uint64_t a = rnd() >> (rnd() % 64), b = rnd() >> (rnd() % 64), e, g = 5151; u = g;
        int ovf = __builtin_mul_overflow(a, b, &e);
        r = mul_checked_u(a, b, &u);
        if (ovf ? (r != 0 || u != g) : (r != 1 || u != e)) ok = 0;
    }
    CHECK("mul_checked_u random", ok, "a random pair was classified wrongly");

    uint64_t fact[22]; fact[0] = 1; for (int i = 1; i <= 20; i++) fact[i] = fact[i - 1] * i;
    r = fact_checked(0, &u);  CHECK("0! = 1", r == 1 && u == 1, "returned %d, *out = %lu", r, u);
    r = fact_checked(1, &u);  CHECK("1! = 1", r == 1 && u == 1, "returned %d, *out = %lu", r, u);
    r = fact_checked(5, &u);  CHECK("5! = 120", r == 1 && u == 120, "returned %d, *out = %lu", r, u);
    ok = 1; unsigned bn = 0;
    for (unsigned n = 0; n <= 20; n++) { u = 0; if (fact_checked(n, &u) != 1 || u != fact[n]) { ok = 0; bn = n; } }
    CHECK("0! to 20! all correct", ok, "wrong for n = %u", bn);
    u = 31; r = fact_checked(21, &u);  CHECK("21! overflows", r == 0 && u == 31, "returned %d, *out = %lu", r, u);
    u = 31; r = fact_checked(100, &u); CHECK("100! overflows", r == 0 && u == 31, "returned %d, *out = %lu", r, u);
    return 0;
}
