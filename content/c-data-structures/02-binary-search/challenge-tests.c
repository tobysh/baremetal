typedef struct { long threshold; long lo, hi; int calls; int outside; } Probe;

static int probe(int i, void *ctx) {
    Probe *p = ctx;
    p->calls++;
    if (i < p->lo || i >= p->hi) p->outside++;
    return i >= p->threshold;
}

static int budget(long n) {
    int k = 0;
    while (((long)1 << k) < n + 1) k++;
    return k;
}

static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

static int run(int lo, int hi, long threshold, Probe *p) {
    p->threshold = threshold; p->lo = lo; p->hi = hi; p->calls = 0; p->outside = 0;
    return first_true(lo, hi, probe, p);
}

int main(void) {
    Probe p;
    int r = run(0, 100, 37, &p);
    CHECK("finds the flip point in 0..99", r == 37, "got %d, expected 37", r);
    CHECK("stays inside the range", p.outside == 0, "pred was called %d times with an index outside lo..hi-1", p.outside);

    r = run(0, 100, 1000, &p);
    CHECK("never true returns hi", r == 100, "got %d, expected 100", r);
    r = run(0, 100, -5, &p);
    CHECK("always true returns lo", r == 0, "got %d, expected 0", r);
    r = run(-50, 50, -7, &p);
    CHECK("negative ranges work", r == -7, "got %d, expected -7", r);

    r = run(5, 5, 0, &p);
    CHECK("an empty range returns lo without calling pred", r == 5 && p.calls == 0, "got %d after %d calls", r, p.calls);
    r = run(9, 3, 0, &p);
    CHECK("lo > hi is empty too", r == 9 && p.calls == 0, "got %d after %d calls", r, p.calls);

    r = run(0, 1000000, 765432, &p);
    CHECK("a million values need only about 20 calls", r == 765432 && p.calls <= budget(1000000), "got %d after %d calls (limit %d)", r, p.calls, budget(1000000));

    r = run(1500000000, 2100000000, 2000000001L, &p);
    CHECK("lo + hi does not overflow", r == 2000000001 && p.outside == 0, "got %d (is lo + hi overflowing?)", r);

    r = run(INT_MIN, INT_MAX, -123456789L, &p);
    CHECK("the whole int range", r == -123456789 && p.calls <= 32, "got %d after %d calls", r, p.calls);
    r = run(INT_MIN, INT_MAX, 2147483647L, &p);
    CHECK("true only at the very top", r == INT_MAX && p.outside == 0, "got %d", r);

    unsigned seed = 4242u;
    int bad = 0, slow = 0;
    for (int t = 0; t < 200; t++) {
        int lo = (int)(rng(&seed) % 2001u) - 1000;
        int n = (int)(rng(&seed) % 300u);
        long th = lo - 3 + (long)(rng(&seed) % (unsigned)(n + 7));
        int want = th <= lo ? lo : (th >= lo + n ? lo + n : (int)th);
        r = run(lo, lo + n, th, &p);
        if (r != want || p.outside) bad++;
        if (p.calls > budget(n)) slow++;
    }
    CHECK("200 random ranges and thresholds", bad == 0, "%d of 200 random cases gave the wrong index", bad);
    CHECK("the call budget holds on every random range", slow == 0, "%d of 200 random cases used too many calls", slow);
    return 0;
}
