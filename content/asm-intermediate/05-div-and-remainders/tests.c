extern uint64_t divmod_u(uint64_t n, uint64_t d, uint64_t *rem);
extern long floor_div(long a, long b);
extern long mod_pos(long a, long m);

static uint64_t seed = 777;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 27); }

int main(void) {
    uint64_t q, rem = 0xdead;
    q = divmod_u(17, 5, &rem);  CHECK("17 / 5 = 3 r 2", q == 3 && rem == 2, "got %lu r %lu", q, rem);
    q = divmod_u(20, 5, &rem);  CHECK("exact division has remainder 0", q == 4 && rem == 0, "got %lu r %lu", q, rem);
    q = divmod_u(3, 9, &rem);   CHECK("n < d", q == 0 && rem == 3, "got %lu r %lu", q, rem);
    q = divmod_u(~0UL, 10, &rem); CHECK("values above 2^63 are unsigned", q == ~0UL / 10 && rem == ~0UL % 10, "got %lu r %lu", q, rem);
    int ok = 1; uint64_t bn = 0, bd = 0;
    for (int i = 0; i < 300 && ok; i++) {
        uint64_t n = rnd(), d = (rnd() >> (rnd() % 60)) | 1;
        uint64_t qq = divmod_u(n, d, &rem);
        if (qq != n / d || rem != n % d) { ok = 0; bn = n; bd = d; }
    }
    CHECK("divmod_u random", ok, "wrong for %lu / %lu", bn, bd);

    long f;
    f = floor_div(7, 2);    CHECK("floor_div(7, 2) = 3", f == 3, "got %ld", f);
    f = floor_div(-7, 2);   CHECK("floor_div(-7, 2) = -4", f == -4, "got %ld", f);
    f = floor_div(7, -2);   CHECK("floor_div(7, -2) = -4", f == -4, "got %ld", f);
    f = floor_div(-7, -2);  CHECK("floor_div(-7, -2) = 3", f == 3, "got %ld", f);
    f = floor_div(-8, 2);   CHECK("exact negative division", f == -4, "got %ld", f);
    f = floor_div(0, -5);   CHECK("floor_div(0, -5) = 0", f == 0, "got %ld", f);
    ok = 1; long ba = 0, bb = 0;
    for (int i = 0; i < 400 && ok; i++) {
        long a = (long)rnd() >> (rnd() % 50), b = (long)(rnd() >> (rnd() % 62)); if (rnd() & 1) b = -b; if (b == 0) b = 3;
        long e = a / b; if ((a % b != 0) && ((a % b < 0) != (b < 0))) e--;
        if (floor_div(a, b) != e) { ok = 0; ba = a; bb = b; }
    }
    CHECK("floor_div random", ok, "wrong for %ld / %ld", ba, bb);

    long m;
    m = mod_pos(7, 5);     CHECK("mod_pos(7, 5) = 2", m == 2, "got %ld", m);
    m = mod_pos(-7, 5);    CHECK("mod_pos(-7, 5) = 3", m == 3, "got %ld", m);
    m = mod_pos(-10, 5);   CHECK("mod_pos(-10, 5) = 0", m == 0, "got %ld", m);
    m = mod_pos(0, 9);     CHECK("mod_pos(0, 9) = 0", m == 0, "got %ld", m);
    ok = 1;
    for (int i = 0; i < 400 && ok; i++) {
        long a = (long)rnd() >> (rnd() % 40), mm = (long)(rnd() >> (rnd() % 62)) | 1;
        long e = a % mm; if (e < 0) e += mm;
        if (mod_pos(a, mm) != e) { ok = 0; ba = a; bb = mm; }
    }
    CHECK("mod_pos random", ok, "wrong for %ld mod %ld", ba, bb);
    return 0;
}
