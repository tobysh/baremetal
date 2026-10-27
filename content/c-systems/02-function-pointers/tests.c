static int t_double(int x) { return x * 2; }
static int t_negate(int x) { return -x; }
static int t_square(int x) { return x * x; }
static int t_is_even(int x) { return x % 2 == 0; }
static int t_is_positive(int x) { return x > 0; }
static int t_always(int x) { (void)x; return 1; }
static int t_asc(int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); }
static int t_desc(int a, int b) { return a < b ? 1 : (a > b ? -1 : 0); }
static int t_by_last_digit(int a, int b) { return (a % 10) - (b % 10); }

static int same(const int *a, const int *b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

int main(void) {
    int a[] = {1, 2, 3, 4};
    int want_double[] = {2, 4, 6, 8};
    map_ints(a, 4, t_double);
    CHECK("map_ints with doubling gives {2, 4, 6, 8}", same(a, want_double, 4), "got %d %d %d %d", a[0], a[1], a[2], a[3]);

    int b[] = {3, -4, 5};
    int want_neg[] = {-3, 4, -5};
    map_ints(b, 3, t_negate);
    CHECK("map_ints uses whichever function it is given", same(b, want_neg, 3), "got %d %d %d", b[0], b[1], b[2]);

    int c[] = {7, 7, 9};
    map_ints(c, 0, t_square);
    CHECK("map_ints with n = 0 changes nothing", c[0] == 7 && c[1] == 7 && c[2] == 9, "array was modified");

    int d[] = {1, 2, 3, 4, 5, 6};
    CHECK("count_if counts the even numbers", count_if(d, 6, t_is_even) == 3, "got %d", count_if(d, 6, t_is_even));
    int e[] = {-2, 0, 4, -9, 8};
    CHECK("count_if counts the positive numbers", count_if(e, 5, t_is_positive) == 2, "got %d", count_if(e, 5, t_is_positive));
    CHECK("count_if with an always-true test counts everything", count_if(e, 5, t_always) == 5, "got %d", count_if(e, 5, t_always));
    CHECK("count_if of an empty array is 0", count_if(e, 0, t_always) == 0, "got %d", count_if(e, 0, t_always));

    int f[] = {5, 2, 9, 1, 5, 6};
    int want_asc[] = {1, 2, 5, 5, 6, 9};
    sort_ints(f, 6, t_asc);
    CHECK("sort_ints ascending", same(f, want_asc, 6), "got %d %d %d %d %d %d", f[0], f[1], f[2], f[3], f[4], f[5]);

    int g[] = {5, 2, 9, 1, 5, 6};
    int want_desc[] = {9, 6, 5, 5, 2, 1};
    sort_ints(g, 6, t_desc);
    CHECK("sort_ints descending with a different callback", same(g, want_desc, 6), "got %d %d %d %d %d %d", g[0], g[1], g[2], g[3], g[4], g[5]);

    int h[] = {23, 41, 12, 55, 9};
    sort_ints(h, 5, t_by_last_digit);
    int ok = 1;
    for (int i = 0; i + 1 < 5; i++) if (t_by_last_digit(h[i], h[i + 1]) > 0) ok = 0;
    CHECK("sort_ints orders by whatever the callback says (last digit)", ok, "got %d %d %d %d %d", h[0], h[1], h[2], h[3], h[4]);

    int one[] = {42};
    sort_ints(one, 1, t_asc);
    sort_ints(one, 0, t_asc);
    CHECK("sort_ints copes with 0 and 1 elements", one[0] == 42, "got %d", one[0]);

    int big[200];
    for (int i = 0; i < 200; i++) big[i] = (i * 37) % 101;
    sort_ints(big, 200, t_asc);
    ok = 1;
    for (int i = 0; i + 1 < 200; i++) if (big[i] > big[i + 1]) ok = 0;
    CHECK("sort_ints sorts 200 numbers", ok, "array is not in order");
    return 0;
}
