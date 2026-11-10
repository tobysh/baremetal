int array_sum(int a[], int n);
int array_max(int a[], int n);
int count_above(int a[], int n, int limit);

/* Each check shows the whole array and n, so "only the first n elements" is visible. */
static void begin(const char *name, int seeded, const int *a, int shown, int n) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_ARR("a", a, shown);
    BM_IN_I("n", n);
}

static void sum_case(const char *name, int seeded, int *a, int shown, int n) {
    begin(name, seeded, a, shown, n);
    BM_OUT_I("array_sum(a, n)", array_sum(a, n));
}

static void max_case(const char *name, int seeded, int *a, int shown, int n) {
    begin(name, seeded, a, shown, n);
    BM_OUT_I("array_max(a, n)", array_max(a, n));
}

static void above_case(const char *name, int seeded, int *a, int shown, int n, int limit) {
    begin(name, seeded, a, shown, n);
    BM_IN_I("limit", limit);
    BM_OUT_I("count_above(a, n, limit)", count_above(a, n, limit));
}

BM_DRIVER {
    int a[5] = {70, 85, 90, 60, 100};
    int b[3] = {-4, 10, -1};
    int c[1] = {42};
    int d[4] = {-9, -3, -7, -20};
    int e[4] = {9, 1, 2, 3};
    int f[4] = {1, 2, 3, 9};

    sum_case("array_sum of 70 85 90 60 100 is 405", 0, a, 5, 5);
    sum_case("array_sum handles negatives", 0, b, 3, 3);
    sum_case("array_sum of one element", 0, c, 1, 1);
    sum_case("array_sum of an empty array is 0", 0, a, 5, 0);
    sum_case("array_sum uses only the first n elements", 0, a, 5, 2);

    max_case("array_max of 70 85 90 60 100 is 100", 0, a, 5, 5);
    max_case("array_max works when all are negative", 0, d, 4, 4);
    max_case("array_max finds a maximum at the first index", 0, e, 4, 4);
    max_case("array_max finds a maximum at the last index", 0, f, 4, 4);
    max_case("array_max of one element", 0, c, 1, 1);
    max_case("array_max uses only the first n elements", 0, a, 5, 2);

    above_case("count_above(a, 5, 80) is 3", 0, a, 5, 5, 80);
    above_case("count_above is strictly greater", 0, a, 5, 5, 90);
    above_case("count_above with a huge limit is 0", 0, a, 5, 5, 1000);
    above_case("count_above with a tiny limit counts everything", 0, d, 4, 4, -100);
    above_case("count_above of an empty array is 0", 0, a, 5, 0, 0);

    static const char *const sums[] = {"array_sum of a random array (1)", "array_sum of a random array (2)", "array_sum of a random array (3)"};
    static const char *const maxes[] = {"array_max of a random array (1)", "array_max of a random array (2)", "array_max of a random array (3)"};
    static const char *const aboves[] = {"count_above on a random array (1)", "count_above on a random array (2)", "count_above on a random array (3)"};
    int r[20];
    for (int k = 0; k < 3; k++) {
        int n = (int)bm_int(1, 20);
        bm_fill(r, n, -500, 500);
        sum_case(sums[k], 1, r, n, n);
    }
    for (int k = 0; k < 3; k++) {
        int n = (int)bm_int(1, 20);
        bm_fill(r, n, -500, 500);
        max_case(maxes[k], 1, r, n, n);
    }
    int n = (int)bm_int(2, 12);
    bm_fill(r, n, -500, -1);
    max_case("array_max of a random all-negative array", 1, r, n, n);
    for (int k = 0; k < 3; k++) {
        n = (int)bm_int(1, 20);
        bm_fill(r, n, -500, 500);
        /* A limit taken from the array itself tests "strictly greater". */
        above_case(aboves[k], 1, r, n, n, r[bm_int(0, n - 1)]);
    }
    n = (int)bm_int(3, 20);
    bm_fill(r, n, 0, 100);
    sum_case("array_sum of a random prefix", 1, r, n, (int)bm_int(1, n - 1));
}
