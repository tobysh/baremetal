static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    int s1[] = {1, 2, 2, 5, 9};
    int s2[] = {1, 3, 2};
    int s3[] = {4, 3};
    int s4[] = {-9, -9, -9};
    CHECK("is_sorted accepts an ascending array with a duplicate", is_sorted(s1, 5) == 1, "got %d", is_sorted(s1, 5));
    CHECK("is_sorted rejects 1 3 2", is_sorted(s2, 3) == 0, "got %d", is_sorted(s2, 3));
    CHECK("is_sorted rejects 4 3", is_sorted(s3, 2) == 0, "got %d", is_sorted(s3, 2));
    CHECK("is_sorted accepts all-equal, one element and empty", is_sorted(s4, 3) == 1 && is_sorted(s4, 1) == 1 && is_sorted(NULL, 0) == 1, "wrong");

    int a[] = {9, 1, 5, 3, 7};
    int orig[] = {9, 1, 5, 3, 7};
    CHECK("median of 9 1 5 3 7 is 5", median_of(a, 5) == 5, "got %d", median_of(a, 5));
    CHECK("median leaves the input order alone", memcmp(a, orig, sizeof a) == 0, "the array was reordered");
    int ev[] = {40, 10, 30, 20};
    CHECK("median of an even count is the lower middle (20)", median_of(ev, 4) == 20, "got %d", median_of(ev, 4));
    int one[] = {-3};
    CHECK("median of one element", median_of(one, 1) == -3, "got %d", median_of(one, 1));
    int dup[] = {2, 2, 2, 9, 9};
    CHECK("median with duplicates is 2", median_of(dup, 5) == 2, "got %d", median_of(dup, 5));

    CHECK("count_in_range [3, 7] on 9 1 5 3 7 is 3", count_in_range(a, 5, 3, 7) == 3, "got %d", count_in_range(a, 5, 3, 7));
    CHECK("count_in_range includes both limits", count_in_range(a, 5, 9, 9) == 1 && count_in_range(a, 5, 1, 1) == 1, "got %d and %d", count_in_range(a, 5, 9, 9), count_in_range(a, 5, 1, 1));
    CHECK("count_in_range with lo > hi is 0", count_in_range(a, 5, 7, 3) == 0, "got %d", count_in_range(a, 5, 7, 3));
    CHECK("count_in_range outside the data is 0", count_in_range(a, 5, 10, 20) == 0 && count_in_range(a, 5, -20, 0) == 0, "wrong");
    CHECK("count_in_range covering everything counts all", count_in_range(a, 5, -1000, 1000) == 5, "got %d", count_in_range(a, 5, -1000, 1000));
    int rep[] = {4, 4, 1, 4, 8, 4};
    CHECK("count_in_range counts duplicates", count_in_range(rep, 6, 4, 4) == 4, "got %d", count_in_range(rep, 6, 4, 4));
    CHECK("count_in_range on an empty array is 0", count_in_range(NULL, 0, 0, 10) == 0, "got %d", count_in_range(NULL, 0, 0, 10));
    CHECK("count_in_range leaves the input order alone", memcmp(a, orig, sizeof a) == 0, "the array was reordered");

    unsigned seed = 1618u;
    int ok = 1, trial = 0;
    for (; trial < 80 && ok; trial++) {
        int n = 1 + (int)(rng(&seed) % 60u);
        int x[60], y[60];
        for (int i = 0; i < n; i++) x[i] = y[i] = (int)(rng(&seed) % 41u) - 20;
        int lo = (int)(rng(&seed) % 51u) - 25, hi = (int)(rng(&seed) % 51u) - 25;
        int want = 0;
        for (int i = 0; i < n; i++) if (x[i] >= lo && x[i] <= hi) want++;
        int smaller = 0;
        int m = median_of(x, n);
        for (int i = 0; i < n; i++) if (x[i] < m) smaller++;
        int le = 0;
        for (int i = 0; i < n; i++) if (x[i] <= m) le++;
        if (count_in_range(x, n, lo, hi) != want) ok = 0;
        if (!(smaller <= (n - 1) / 2 && le >= (n - 1) / 2 + 1)) ok = 0;
        if (memcmp(x, y, (size_t)n * sizeof(int)) != 0) ok = 0;
    }
    CHECK("80 random arrays: counts, medians and untouched inputs", ok, "failed on trial %d", trial);
    return 0;
}
