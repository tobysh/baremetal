static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

static long slow_inversions(const int *a, int n) {
    long c = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (a[i] > a[j]) c++;
    return c;
}

int main(void) {
    int a[] = {5, 2, 4, 6, 1, 3};
    CHECK("the lesson's example has 9 inversions", count_inversions(a, 6) == 9, "got %ld", count_inversions(a, 6));
    CHECK("the input is not modified", a[0] == 5 && a[1] == 2 && a[2] == 4 && a[3] == 6 && a[4] == 1 && a[5] == 3, "the array was changed, sort a copy");

    int sorted[] = {1, 2, 3, 4, 5};
    int rev[] = {5, 4, 3, 2, 1};
    int same[] = {7, 7, 7, 7};
    CHECK("a sorted array has none", count_inversions(sorted, 5) == 0, "got %ld", count_inversions(sorted, 5));
    CHECK("a reversed array of 5 has 10", count_inversions(rev, 5) == 10, "got %ld", count_inversions(rev, 5));
    CHECK("equal values are not inversions", count_inversions(same, 4) == 0, "got %ld", count_inversions(same, 4));
    CHECK("n = 0 and n = 1", count_inversions(NULL, 0) == 0 && count_inversions(rev, 1) == 0, "got %ld and %ld", count_inversions(NULL, 0), count_inversions(rev, 1));

    int mixed[] = {-3, 8, -3, 0, 2147483647, -2147483647 - 1, 0};
    long want = slow_inversions(mixed, 7);
    CHECK("negatives, duplicates and INT_MIN/INT_MAX", count_inversions(mixed, 7) == want, "got %ld, expected %ld (compare, do not subtract)", count_inversions(mixed, 7), want);

    unsigned seed = 99u;
    int bad = 0;
    for (int t = 0; t < 100; t++) {
        int n = (int)(rng(&seed) % 60u);
        int v[60];
        for (int i = 0; i < n; i++) v[i] = (int)(rng(&seed) % 25u) - 12;
        if (count_inversions(v, n) != slow_inversions(v, n)) bad++;
    }
    CHECK("100 random small arrays match a brute-force count", bad == 0, "%d of 100 arrays were counted wrongly", bad);

    int n = 200000;
    int *big = malloc((size_t)n * sizeof *big);
    for (int i = 0; i < n; i++) big[i] = n - i;
    long expect = (long)n * (n - 1) / 2;
    long got = count_inversions(big, n);
    CHECK("200000 descending values have n(n-1)/2 inversions", got == expect, "got %ld, expected %ld", got, expect);
    for (int i = 0; i < n; i++) big[i] = (int)(rng(&seed) % 1000u);
    got = count_inversions(big, n);
    CHECK("200000 random values with many duplicates finish in time", got > 0 && got < expect, "got %ld", got);
    free(big);
    return 0;
}
