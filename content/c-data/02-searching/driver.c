#include <limits.h>

int index_of(int a[], int n, int target);
int last_index_of(int a[], int n, int target);
int count_value(int a[], int n, int target);
int is_sorted(int a[], int n);

static void search_case(const char *name, int seeded, int *a, int shown, int n, int target) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_ARR("a", a, shown);
    BM_IN_I("n", n);
    BM_IN_I("target", target);
    /* Each function receives the same original values, regardless of side effects. */
    int copy[32];
    for (int i = 0; i < shown; i++) copy[i] = a[i];
    BM_OUT_I("index_of(a, n, target)", index_of(copy, n, target));
    for (int i = 0; i < shown; i++) copy[i] = a[i];
    BM_OUT_I("last_index_of(a, n, target)", last_index_of(copy, n, target));
    for (int i = 0; i < shown; i++) copy[i] = a[i];
    BM_OUT_I("count_value(a, n, target)", count_value(copy, n, target));
}

static void sorted_case(const char *name, int seeded, int *a, int shown, int n) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_ARR("a", a, shown);
    BM_IN_I("n", n);
    BM_OUT_I("is_sorted(a, n)", is_sorted(a, n));
}

BM_DRIVER {
    int a[6] = {4, 8, 15, 8, 23, 8};
    int zero[4] = {0, 0, 0, 0};
    int one[1] = {42};
    int negative[5] = {-7, -2, -7, -1, -7};
    int extreme[4] = {INT_MIN, -1, 0, INT_MAX};
    search_case("search repeated matches", 0, a, 6, 6, 8);
    search_case("search the first element", 0, a, 6, 6, 4);
    search_case("search a single interior match", 0, a, 6, 6, 15);
    search_case("search a match near the end", 0, a, 6, 6, 23);
    search_case("search an absent value", 0, a, 6, 6, 99);
    search_case("search only the first n elements", 0, a, 6, 3, 23);
    search_case("last match and count respect n", 0, a, 6, 4, 8);
    search_case("count respects a shorter prefix", 0, a, 6, 2, 8);
    search_case("search an empty prefix", 0, a, 6, 0, 4);
    search_case("search zero values", 0, zero, 4, 4, 0);
    search_case("search a one-element array", 0, one, 1, 1, 42);
    search_case("search repeated negative values", 0, negative, 5, 5, -7);
    search_case("search extreme int values", 0, extreme, 4, 4, INT_MIN);

    int repeated[5] = {1, 2, 2, 7, 9};
    int drop[5] = {1, 2, 3, 9, 8};
    int descending[3] = {5, 4, 3};
    sorted_case("is_sorted accepts repeated values", 0, repeated, 5, 5);
    sorted_case("is_sorted rejects a drop at the end", 0, drop, 5, 5);
    sorted_case("is_sorted rejects descending data", 0, descending, 3, 3);
    sorted_case("is_sorted ignores elements past n", 0, drop, 5, 4);
    sorted_case("is_sorted of one element is 1", 0, descending, 3, 1);
    sorted_case("is_sorted of an empty prefix is 1", 0, descending, 3, 0);
    sorted_case("is_sorted handles the full int range", 0, extreme, 4, 4);

    static const char *const names[] = {
        "search a fresh array (1)", "search a fresh array (2)",
        "search a fresh array (3)", "search a fresh array (4)",
        "search a fresh array (5)", "search a fresh array (6)",
    };
    for (int k = 0; k < 6; k++) {
        int values[32];
        int n = (int)bm_int(2, 32);
        bm_fill(values, n, -6, 6);
        int target = values[bm_int(0, n - 1)];
        search_case(names[k], 1, values, n, n, target);
    }
    int values[32];
    bm_fill(values, 32, -100, 100);
    int prefix = (int)bm_int(1, 30);
    search_case("search a fresh absent value", 1, values, 32, 32, 200);
    search_case("search a fresh prefix", 1, values, 32, prefix, values[31]);

    int increasing[20];
    increasing[0] = (int)bm_int(-500, -100);
    for (int i = 1; i < 20; i++) increasing[i] = increasing[i - 1] + (int)bm_int(0, 10);
    sorted_case("is_sorted accepts fresh nondecreasing values", 1, increasing, 20, 20);
    int unsorted[20];
    bm_fill(unsorted, 20, -100, 100);
    unsorted[18] = (int)bm_int(10, 100);
    unsorted[19] = (int)bm_int(-100, -10);
    sorted_case("is_sorted rejects a fresh array with a final drop", 1, unsorted, 20, 20);
}
