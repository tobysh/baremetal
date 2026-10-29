extern void add128(const uint64_t *a, const uint64_t *b, uint64_t *out);
extern uint64_t mulhi(uint64_t a, uint64_t b);
extern unsigned add_n(const uint64_t *a, const uint64_t *b, uint64_t *out, size_t n);

static uint64_t seed = 2718;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 28); }

int main(void) {
    uint64_t a[2] = { ~0ULL, 0 }, b[2] = { 1, 0 }, o[2] = { 9, 9 };
    add128(a, b, o);  CHECK("carry from low to high limb", o[0] == 0 && o[1] == 1, "got {%lu, %lu}", o[0], o[1]);
    uint64_t c[2] = { ~0ULL, ~0ULL };
    add128(c, b, o);  CHECK("wraps at 128 bits", o[0] == 0 && o[1] == 0, "got {%lu, %lu}", o[0], o[1]);
    uint64_t d[2] = { 5, 7 }, e[2] = { 6, 8 };
    add128(d, e, o);  CHECK("no carry", o[0] == 11 && o[1] == 15, "got {%lu, %lu}", o[0], o[1]);
    uint64_t f[2] = { ~0ULL, 3 };
    add128(f, f, f);  CHECK("out aliasing both inputs", f[0] == ~0ULL - 1 && f[1] == 7, "got {%lu, %lu}", f[0], f[1]);
    int ok = 1;
    for (int i = 0; i < 200 && ok; i++) {
        uint64_t x[2] = { rnd(), rnd() }, y[2] = { rnd(), rnd() }, z[2];
        if (i & 1) x[0] = ~0ULL - (rnd() & 3);
        unsigned __int128 s = (((unsigned __int128)x[1] << 64) | x[0]) + (((unsigned __int128)y[1] << 64) | y[0]);
        add128(x, y, z);
        if (z[0] != (uint64_t)s || z[1] != (uint64_t)(s >> 64)) ok = 0;
    }
    CHECK("add128 random", ok, "a random pair added wrongly");

    uint64_t h;
    h = mulhi(1UL << 63, 4);       CHECK("mulhi(2^63, 4) = 2", h == 2, "got %lu", h);
    h = mulhi(5, 7);               CHECK("small product has high half 0", h == 0, "got %lu", h);
    h = mulhi(~0UL, ~0UL);         CHECK("mulhi(max, max)", h == ~0UL - 1, "got %lu", h);
    ok = 1;
    for (int i = 0; i < 300 && ok; i++) {
        uint64_t x = rnd(), y = rnd();
        if (mulhi(x, y) != (uint64_t)(((unsigned __int128)x * y) >> 64)) ok = 0;
    }
    CHECK("mulhi random", ok, "wrong for a random pair");

    uint64_t p[5] = { ~0ULL, ~0ULL, ~0ULL, ~0ULL, ~0ULL }, q[5] = { 1, 0, 0, 0, 0 }, r[5];
    unsigned cy = add_n(p, q, r, 5);
    CHECK("carry ripples through 5 limbs", cy == 1 && r[0] == 0 && r[1] == 0 && r[4] == 0, "carry %u, r[0]=%lu r[4]=%lu", cy, r[0], r[4]);
    uint64_t s1[3] = { 1, 2, 3 }, s2[3] = { 10, 20, 30 };
    cy = add_n(s1, s2, r, 3);
    CHECK("no carry out", cy == 0 && r[0] == 11 && r[1] == 22 && r[2] == 33, "carry %u, r = {%lu, %lu, %lu}", cy, r[0], r[1], r[2]);
    r[0] = 77; cy = add_n(s1, s2, r, 0);
    CHECK("n = 0 returns 0 and writes nothing", cy == 0 && r[0] == 77, "carry %u, r[0]=%lu", cy, r[0]);
    ok = 1;
    for (int t = 0; t < 100 && ok; t++) {
        size_t n = 1 + rnd() % 12; uint64_t x[12], y[12], z[12]; unsigned __int128 carry = 0; uint64_t ez[12];
        for (size_t i = 0; i < n; i++) { x[i] = rnd(); y[i] = rnd(); if (rnd() % 3 == 0) x[i] = ~0ULL; if (rnd() % 3 == 0) y[i] = 0; }
        for (size_t i = 0; i < n; i++) { unsigned __int128 s = (unsigned __int128)x[i] + y[i] + carry; ez[i] = (uint64_t)s; carry = s >> 64; }
        unsigned got = add_n(x, y, z, n);
        if (got != (unsigned)carry || memcmp(z, ez, n * 8)) ok = 0;
    }
    CHECK("add_n random lengths", ok, "a random addition was wrong (is CF preserved inside the loop?)");
    return 0;
}
