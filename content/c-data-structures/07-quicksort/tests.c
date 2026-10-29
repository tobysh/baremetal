static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static int sorted(const int *a, size_t n) { for (size_t i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0; return 1; }
static int cmp(const void *x, const void *y) { int a = *(const int *)x, b = *(const int *)y; return (a > b) - (a < b); }

int main(void) {
    int a[] = {9, 4, 7, 1, 8, 2, 6};
    size_t p = partition(a, 7);
    int pv = 6, ok = p < 7 && a[p] == pv;
    for (size_t i = 0; ok && i < p; i++) if (a[i] > pv) ok = 0;
    for (size_t i = p + 1; ok && i < 7; i++) if (a[i] < pv) ok = 0;
    CHECK("partition around the pivot 6", ok, "returned %zu: %d %d %d %d %d %d %d", p, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
    CHECK("the pivot 6 ends at index 4 (four values are smaller)", p == 4, "returned %zu", p);
    long sum = 0;
    for (int i = 0; i < 7; i++) sum += a[i];
    CHECK("partition keeps the same elements", sum == 37, "sum is %ld, expected 37", sum);

    int one[] = {5};
    CHECK("partition of one element returns 0", partition(one, 1) == 0 && one[0] == 5, "wrong");
    int small[] = {1, 2, 3};
    int big3[] = {9, 8, 0};
    CHECK("pivot that is the largest returns n-1", partition(small, 3) == 2 && small[2] == 3, "wrong");
    CHECK("pivot that is the smallest returns 0", partition(big3, 3) == 0 && big3[0] == 0, "wrong");
    int eq[] = {4, 4, 4, 4, 4};
    p = partition(eq, 5);
    CHECK("partition of equal values keeps them", p < 5 && eq[0] == 4 && eq[4] == 4, "returned %zu", p);

    int t[] = {3, 1, 2};
    quicksort(t, 3);
    CHECK("quicksort sorts three elements", t[0] == 1 && t[1] == 2 && t[2] == 3, "got %d %d %d", t[0], t[1], t[2]);
    quicksort(t, 0);
    quicksort(t, 1);
    CHECK("quicksort with n = 0 or 1 does nothing", t[0] == 1 && t[2] == 3, "array changed");

    unsigned seed = 2718u;
    ok = 1;
    int trial = 0;
    for (; trial < 50 && ok; trial++) {
        size_t n = rng(&seed) % 150u;
        int x[150], y[150];
        for (size_t i = 0; i < n; i++) x[i] = y[i] = (int)(rng(&seed) % 61u) - 30;
        quicksort(x, n);
        qsort(y, n, sizeof(int), cmp);
        if (memcmp(x, y, n * sizeof(int)) != 0) ok = 0;
    }
    CHECK("50 random arrays match the library sort", ok, "failed on trial %d", trial);

    size_t N = 4000;
    int *buf = malloc(N * sizeof *buf);
    for (size_t i = 0; i < N; i++) buf[i] = (int)i;
    quicksort(buf, N);
    CHECK("4000 already-sorted values", sorted(buf, N) && buf[0] == 0 && buf[N - 1] == (int)N - 1, "not sorted");
    for (size_t i = 0; i < N; i++) buf[i] = (int)(N - i);
    quicksort(buf, N);
    CHECK("4000 reversed values", sorted(buf, N) && buf[0] == 1, "not sorted");
    for (size_t i = 0; i < N; i++) buf[i] = 7;
    quicksort(buf, N);
    CHECK("4000 equal values", sorted(buf, N) && buf[N - 1] == 7, "not sorted");
    for (size_t i = 0; i < N; i++) buf[i] = (int)(rng(&seed) % 100000u);
    quicksort(buf, N);
    CHECK("4000 random values", sorted(buf, N), "not sorted");

    int ordered[] = {10, 20, 30, 40, 50, 60, 70};
    int rs = 1;
    for (size_t k = 0; k < 7; k++) {
        int w[] = {50, 10, 70, 30, 20, 60, 40};
        if (quickselect(w, 7, k) != ordered[k]) rs = 0;
    }
    CHECK("quickselect finds every rank of a 7-element array", rs, "wrong rank");

    int dups[] = {5, 1, 5, 1, 5, 9, 1};
    int d0 = quickselect(dups, 7, 0);
    int dups2[] = {5, 1, 5, 1, 5, 9, 1};
    int d3 = quickselect(dups2, 7, 3);
    int dups3[] = {5, 1, 5, 1, 5, 9, 1};
    int d6 = quickselect(dups3, 7, 6);
    CHECK("quickselect with duplicates (min, middle, max)", d0 == 1 && d3 == 5 && d6 == 9, "got %d %d %d", d0, d3, d6);

    ok = 1;
    for (int trial2 = 0; trial2 < 30 && ok; trial2++) {
        size_t n = 1 + rng(&seed) % 100u;
        int x[100], y[100];
        for (size_t i = 0; i < n; i++) x[i] = y[i] = (int)(rng(&seed) % 40u) - 20;
        size_t k = rng(&seed) % n;
        qsort(y, n, sizeof(int), cmp);
        if (quickselect(x, n, k) != y[k]) ok = 0;
    }
    CHECK("30 random quickselects match the sorted order", ok, "wrong value");
    free(buf);
    return 0;
}
