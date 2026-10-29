static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static int sorted(const int *a, size_t n) { for (size_t i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0; return 1; }

int main(void) {
    int a[] = {1, 4, 9};
    int b[] = {2, 3, 10, 11};
    int out[8];
    merge(a, 3, b, 4, out);
    int want[] = {1, 2, 3, 4, 9, 10, 11};
    int ok = 1;
    for (int i = 0; i < 7; i++) if (out[i] != want[i]) ok = 0;
    CHECK("merge interleaves 1 4 9 with 2 3 10 11", ok, "got %d %d %d %d %d %d %d", out[0], out[1], out[2], out[3], out[4], out[5], out[6]);

    int guard[6] = {-1, -1, -1, -1, -1, -1};
    merge(NULL, 0, b, 4, guard);
    CHECK("merge with an empty left input", guard[0] == 2 && guard[3] == 11 && guard[4] == -1, "got %d %d %d %d", guard[0], guard[1], guard[2], guard[3]);
    merge(a, 3, NULL, 0, guard);
    CHECK("merge with an empty right input", guard[0] == 1 && guard[2] == 9 && guard[3] == 11, "got %d %d %d", guard[0], guard[1], guard[2]);
    int hi[] = {10, 20}, lo[] = {1, 2, 3};
    merge(hi, 2, lo, 3, out);
    CHECK("merge when one side is entirely smaller", out[0] == 1 && out[2] == 3 && out[3] == 10 && out[4] == 20, "got %d %d %d %d %d", out[0], out[1], out[2], out[3], out[4]);
    int d1[] = {5, 5, 7}, d2[] = {5, 7, 7};
    merge(d1, 3, d2, 3, out);
    CHECK("merge keeps duplicates", sorted(out, 6) && out[0] == 5 && out[2] == 5 && out[3] == 7 && out[5] == 7, "got %d %d %d %d %d %d", out[0], out[1], out[2], out[3], out[4], out[5]);

    int tiny[] = {3, 1, 2};
    CHECK("merge_sort on three elements", merge_sort(tiny, 3) == 0 && tiny[0] == 1 && tiny[1] == 2 && tiny[2] == 3, "got %d %d %d", tiny[0], tiny[1], tiny[2]);
    int single[] = {42};
    CHECK("merge_sort on 0 and 1 elements", merge_sort(single, 1) == 0 && single[0] == 42 && merge_sort(NULL, 0) == 0, "failed");

    unsigned seed = 9001u;
    ok = 1;
    int trial = 0;
    for (; trial < 40 && ok; trial++) {
        size_t n = rng(&seed) % 200u;
        int arr[200];
        long sum = 0;
        for (size_t i = 0; i < n; i++) { arr[i] = (int)(rng(&seed) % 2001u) - 1000; sum += arr[i]; }
        if (merge_sort(arr, n) != 0 || !sorted(arr, n)) ok = 0;
        long sum2 = 0;
        for (size_t i = 0; i < n; i++) sum2 += arr[i];
        if (sum != sum2) ok = 0;
    }
    CHECK("40 random arrays come out sorted with the same elements", ok, "failed on trial %d", trial);

    size_t N = 50000;
    int *big = malloc(N * sizeof *big);
    for (size_t i = 0; i < N; i++) big[i] = (int)(rng(&seed) % 100000u);
    CHECK("50000 random values", merge_sort(big, N) == 0 && sorted(big, N), "not sorted");
    for (size_t i = 0; i < N; i++) big[i] = (int)(N - i);
    CHECK("50000 reversed values", merge_sort(big, N) == 0 && sorted(big, N) && big[0] == 1 && big[N - 1] == (int)N, "not sorted");
    for (size_t i = 0; i < N; i++) big[i] = 4;
    CHECK("50000 equal values", merge_sort(big, N) == 0 && sorted(big, N) && big[N - 1] == 4, "not sorted");
    free(big);

    int canary[12];
    for (int i = 0; i < 12; i++) canary[i] = -77;
    int mid[] = {9, 3, 7, 1, 8, 2};
    memcpy(canary + 3, mid, sizeof mid);
    merge_sort(canary + 3, 6);
    CHECK("elements outside the range are untouched", canary[2] == -77 && canary[9] == -77 && sorted(canary + 3, 6), "wrote outside the array");

    Rec recs[300];
    int keys_seen = 1;
    for (int i = 0; i < 300; i++) { recs[i].key = (int)(rng(&seed) % 8u); recs[i].order = i; }
    CHECK("merge_sort_recs succeeds", merge_sort_recs(recs, 300) == 0, "returned non-zero");
    ok = 1;
    for (int i = 1; i < 300; i++) {
        if (recs[i - 1].key > recs[i].key) ok = 0;
        if (recs[i - 1].key == recs[i].key && recs[i - 1].order > recs[i].order) { ok = 0; keys_seen = 0; }
    }
    (void)keys_seen;
    CHECK("records with equal keys keep their original order (stable)", ok, "equal keys were reordered");
    return 0;
}
