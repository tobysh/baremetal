static int ceil_log2_t(long n) { int k = 0; while ((1L << k) < n) k++; return k; }

static long brute(const int *a, int na, const int *b, int nb) {
    long f = 0;
    for (int i = 0; i < na; i++)
        for (int j = 0; j < nb; j++) if (a[i] == b[j]) { f++; break; }
    return f;
}

static int abuf[MAXV], bbuf[MAXV], acopy[MAXV], bcopy[MAXV];

// Runs one case; returns 1 when the answer is right, stores comparisons and budget.
static int run(int na, int nb, int mode, unsigned seed, long *cmps, long *budget, long *want, long *got) {
    // mode 0: random overlap; 1: identical; 2: disjoint; 3: b is a subset of a (stride)
    make(abuf, na, 0, 4, seed);
    if (mode == 1) { memcpy(bbuf, abuf, sizeof(int) * (size_t)na); nb = na; }
    else if (mode == 2) { make(bbuf, nb, -100000, 1, seed + 1); for (int i = 0; i < nb; i++) bbuf[i] = -bbuf[i] - 1; for (int i = 0; i < nb / 2; i++) { int t = bbuf[i]; bbuf[i] = bbuf[nb - 1 - i]; bbuf[nb - 1 - i] = t; } }
    else if (mode == 3) { for (int i = 0; i < nb && i * 2 < na; i++) bbuf[i] = abuf[i * 2]; if (nb * 2 > na) nb = (na + 1) / 2; }
    else make(bbuf, nb, 0, 4, seed + 77);
    memcpy(acopy, abuf, sizeof abuf); memcpy(bcopy, bbuf, sizeof bbuf);
    *want = brute(abuf, na, bbuf, nb);
    ncmp = 0;
    *got = count_common(abuf, na, bbuf, nb);
    *cmps = ncmp;
    long small = na < nb ? na : nb, big = na < nb ? nb : na;
    long b1 = (long)na + nb, b2 = small * (ceil_log2_t(big + 1) + 1);
    *budget = small == 0 ? 0 : (b1 < b2 ? b1 : b2);
    int intact = memcmp(abuf, acopy, sizeof abuf) == 0 && memcmp(bbuf, bcopy, sizeof bbuf) == 0;
    return intact && *want == *got;
}

int main(void) {
    long c, bud, want, got;
    int ok;

    ok = run(0, 0, 0, 1, &c, &bud, &want, &got);
    CHECK("two empty arrays", ok && c == 0, "got %ld common with %ld comparisons", got, c);
    ok = run(0, 50, 0, 2, &c, &bud, &want, &got);
    CHECK("an empty first array needs no comparisons", ok && c == 0, "got %ld common with %ld comparisons", got, c);
    ok = run(50, 0, 0, 3, &c, &bud, &want, &got);
    CHECK("an empty second array needs no comparisons", ok && c == 0, "got %ld common with %ld comparisons", got, c);

    ok = run(300, 300, 0, 4, &c, &bud, &want, &got);
    CHECK("small balanced arrays: correct answer", ok, "got %ld, expected %ld (inputs modified or wrong count)", got, want);
    CHECK("small balanced arrays: within budget", c <= bud, "%ld comparisons, budget %ld", c, bud);

    ok = run(2000, 2000, 0, 5, &c, &bud, &want, &got);
    CHECK("large balanced arrays: correct answer", ok, "got %ld, expected %ld", got, want);
    CHECK("large balanced arrays: within budget (merge-like)", c <= bud, "%ld comparisons, budget %ld", c, bud);

    ok = run(1, 4000, 3, 6, &c, &bud, &want, &got);
    CHECK("one value against a huge array: correct", ok, "got %ld, expected %ld", got, want);
    CHECK("one value against a huge array: logarithmic", c <= bud, "%ld comparisons, budget %ld", c, bud);

    ok = run(4000, 40, 0, 7, &c, &bud, &want, &got);
    CHECK("huge array first, tiny second: correct", ok, "got %ld, expected %ld", got, want);
    CHECK("huge array first, tiny second: within budget", c <= bud, "%ld comparisons, budget %ld", c, bud);

    ok = run(60, 3500, 0, 8, &c, &bud, &want, &got);
    CHECK("small first, huge second: correct", ok, "got %ld, expected %ld", got, want);
    CHECK("small first, huge second: within budget", c <= bud, "%ld comparisons, budget %ld", c, bud);

    ok = run(1500, 0, 1, 9, &c, &bud, &want, &got);
    CHECK("identical arrays: every value is common", ok && got == 1500 && c <= bud, "common %ld (want 1500), %ld comparisons, budget %ld", got, c, bud);
    ok = run(900, 700, 2, 10, &c, &bud, &want, &got);
    CHECK("disjoint arrays: nothing is common", ok && got == 0 && c <= bud, "common %ld, %ld comparisons, budget %ld", got, c, bud);
    ok = run(1000, 400, 3, 11, &c, &bud, &want, &got);
    CHECK("one array is a subset of the other", ok && c <= bud, "common %ld (want %ld), %ld comparisons, budget %ld", got, want, c, bud);
    ok = run(2, 3, 0, 12, &c, &bud, &want, &got);
    CHECK("tiny arrays", ok && c <= bud, "common %ld (want %ld), %ld comparisons, budget %ld", got, want, c, bud);
    return 0;
}
