static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static int ref_lower(const int *a, int n, int x) { int i = 0; while (i < n && a[i] < x) i++; return i; }
static int ref_upper(const int *a, int n, int x) { int i = 0; while (i < n && a[i] <= x) i++; return i; }

int main(void) {
    int a[] = {1, 3, 3, 3, 8, 12};
    CHECK("lower_bound of an empty array is 0", lower_bound(NULL, 0, 5) == 0, "got %d", lower_bound(NULL, 0, 5));
    CHECK("upper_bound of an empty array is 0", upper_bound(NULL, 0, 5) == 0, "got %d", upper_bound(NULL, 0, 5));
    CHECK("lower_bound(3) is 1", lower_bound(a, 6, 3) == 1, "got %d", lower_bound(a, 6, 3));
    CHECK("upper_bound(3) is 4", upper_bound(a, 6, 3) == 4, "got %d", upper_bound(a, 6, 3));
    CHECK("lower_bound(8) is 4 and upper_bound(8) is 5", lower_bound(a, 6, 8) == 4 && upper_bound(a, 6, 8) == 5, "got %d and %d", lower_bound(a, 6, 8), upper_bound(a, 6, 8));
    CHECK("a value below everything gives 0", lower_bound(a, 6, -50) == 0 && upper_bound(a, 6, 0) == 0, "got %d and %d", lower_bound(a, 6, -50), upper_bound(a, 6, 0));
    CHECK("a value above everything gives n", lower_bound(a, 6, 13) == 6 && upper_bound(a, 6, 1000) == 6, "got %d and %d", lower_bound(a, 6, 13), upper_bound(a, 6, 1000));
    CHECK("an absent value gives its insertion point", lower_bound(a, 6, 5) == 4 && upper_bound(a, 6, 5) == 4, "got %d and %d", lower_bound(a, 6, 5), upper_bound(a, 6, 5));
    CHECK("count_equal(3) is 3", count_equal(a, 6, 3) == 3, "got %d", count_equal(a, 6, 3));
    CHECK("count_equal of an absent value is 0", count_equal(a, 6, 4) == 0, "got %d", count_equal(a, 6, 4));
    CHECK("count_equal on an empty array is 0", count_equal(NULL, 0, 1) == 0, "got %d", count_equal(NULL, 0, 1));

    int one[] = {7};
    CHECK("single element: bounds around 7", lower_bound(one, 1, 7) == 0 && upper_bound(one, 1, 7) == 1, "got %d and %d", lower_bound(one, 1, 7), upper_bound(one, 1, 7));
    CHECK("single element: 6 and 8", lower_bound(one, 1, 6) == 0 && lower_bound(one, 1, 8) == 1, "got %d and %d", lower_bound(one, 1, 6), lower_bound(one, 1, 8));

    int same[50];
    for (int i = 0; i < 50; i++) same[i] = 9;
    CHECK("an all-equal array: count is n", count_equal(same, 50, 9) == 50 && lower_bound(same, 50, 9) == 0 && upper_bound(same, 50, 9) == 50, "count %d", count_equal(same, 50, 9));

    int big[1000];
    int v = -300;
    unsigned seed = 77u;
    for (int i = 0; i < 1000; i++) { v += (int)(rng(&seed) % 3u); big[i] = v; }
    int ok = 1, bad = 0;
    for (int x = -310; x <= v + 10; x++) {
        if (lower_bound(big, 1000, x) != ref_lower(big, 1000, x) || upper_bound(big, 1000, x) != ref_upper(big, 1000, x)) { ok = 0; bad = x; break; }
    }
    CHECK("1000 sorted values with duplicates match a linear scan for every probe", ok, "first mismatch at x = %d", bad);

    int lim[] = {-2147483647 - 1, -5, 0, 2147483647};
    CHECK("extreme int values work", lower_bound(lim, 4, 2147483647) == 3 && upper_bound(lim, 4, 2147483647) == 4 && lower_bound(lim, 4, -2147483647 - 1) == 0, "got %d", lower_bound(lim, 4, 2147483647));

    int copy[6];
    for (int i = 0; i < 6; i++) copy[i] = a[i];
    count_equal(copy, 6, 3);
    ok = 1;
    for (int i = 0; i < 6; i++) if (copy[i] != a[i]) ok = 0;
    CHECK("the array is not modified", ok, "an element changed");
    return 0;
}
