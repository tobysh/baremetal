// Independent reference: sort a copy, then count the runs. Shares no code with the learner's functions.
static int cmp_int(const void *x, const void *y) {
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static int ref_distinct(const int *a, int n) {
    if (n == 0) return 0;
    int *t = malloc((size_t)n * sizeof *t);
    memcpy(t, a, (size_t)n * sizeof *t);
    qsort(t, (size_t)n, sizeof *t, cmp_int);
    int runs = 1;
    for (int i = 1; i < n; i++) if (t[i] != t[i - 1]) runs++;
    free(t);
    return runs;
}

static unsigned long long rng = 88172645463325252ULL;
static unsigned next_u(void) {
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return (unsigned)(rng >> 11);
}

// Runs one input; returns 1 on a match. Fills result, expected, work.
static int run_one(const int *a, int n, int *got, int *want, long *work) {
    int *copy = malloc((size_t)(n ? n : 1) * sizeof *copy);
    memcpy(copy, a, (size_t)n * sizeof *copy);
    *want = ref_distinct(a, n);
    bm_work = 0;
    *got = distinct_fast(copy, n);
    *work = bm_work;
    int untouched = n == 0 || memcmp(copy, a, (size_t)n * sizeof *copy) == 0;
    free(copy);
    return *got == *want && untouched;
}

int main(void) {
    int got, want;
    long work;
    int buf[600];

    CHECK("an empty array has no distinct values", run_one(buf, 0, &got, &want, &work), "returned %d, expected 0", got);

    buf[0] = 42;
    run_one(buf, 1, &got, &want, &work);
    CHECK("one element is one value", got == 1, "returned %d, expected 1", got);

    int pair[2] = { -3, 3 };
    run_one(pair, 2, &got, &want, &work);
    CHECK("-3 and 3 are different values", got == 2, "returned %d, expected 2", got);

    int ext[4] = { -100, 100, -100, 100 };
    run_one(ext, 4, &got, &want, &work);
    CHECK("the extremes -100 and 100 work", got == 2, "returned %d, expected 2", got);

    int zeros[5] = { 0, 0, 0, 0, 0 };
    run_one(zeros, 5, &got, &want, &work);
    CHECK("all zeros is one value", got == 1, "returned %d, expected 1", got);

    for (int i = 0; i < 201; i++) buf[i] = i - 100;
    run_one(buf, 201, &got, &want, &work);
    CHECK("every value once gives 201", got == 201, "returned %d, expected 201", got);

    int bad = 0, badn = -1, bgot = 0, bwant = 0;
    for (int t = 0; t < 400; t++) {
        int n = (int)(next_u() % 40);
        int span = 1 + (int)(next_u() % 201);
        int lo = -100 + (int)(next_u() % (unsigned)(202 - span));
        for (int i = 0; i < n; i++) buf[i] = lo + (int)(next_u() % (unsigned)span);
        if (!run_one(buf, n, &got, &want, &work) && !bad) { bad = 1; badn = n; bgot = got; bwant = want; }
    }
    CHECK("400 small random arrays agree with the reference", !bad, "n=%d: returned %d, expected %d", badn, bgot, bwant);

    bad = 0;
    for (int t = 0; t < 60; t++) {
        int n = 100 + (int)(next_u() % 500);
        for (int i = 0; i < n; i++) buf[i] = -100 + (int)(next_u() % 201);
        if (!run_one(buf, n, &got, &want, &work) && !bad) { bad = 1; badn = n; bgot = got; bwant = want; }
    }
    CHECK("60 large random arrays agree with the reference", !bad, "n=%d: returned %d, expected %d", badn, bgot, bwant);

    int n = 500;
    for (int i = 0; i < n; i++) buf[i] = -100 + (int)(next_u() % 201);
    run_one(buf, n, &got, &want, &work);
    CHECK("work stays within 2*n + 256 for n = 500", work <= 2L * n + 256, "work was %ld, budget %ld", work, 2L * n + 256);
    CHECK("the counter counts every element", work >= n, "work was %ld, expected at least %d", work, n);

    for (int i = 0; i < 6; i++) buf[i] = -2 + i;
    run_one(buf, 6, &got, &want, &work);
    CHECK("work stays small for a short array", work <= 2L * 6 + 256, "work was %ld, budget %ld", work, 2L * 6 + 256);

    bm_work = 0;
    int base_ok = 1;
    for (int i = 0; i < 50; i++) buf[i] = (i * 7) % 23 - 11;
    base_ok = distinct_baseline(buf, 50) == ref_distinct(buf, 50);
    CHECK("the baseline was left unchanged", base_ok, "distinct_baseline no longer matches the reference");
    return 0;
}
