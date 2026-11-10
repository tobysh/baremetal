#include <stddef.h>

int array_sum(const int *arr, size_t len);
int array_max(const int *arr, size_t len);

/* Each check shows the whole array and len, so "only the first len elements" is visible. */
static void sum_case(const char *name, int seeded, const int *arr, size_t shown, size_t len) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_ARR("arr", arr, shown);
    BM_IN_U("len", len);
    BM_OUT_I("array_sum(arr, len)", array_sum(arr, len));
    BM_OUT_ARR("arr afterwards", arr, shown);
}

static void max_case(const char *name, int seeded, const int *arr, size_t shown, size_t len) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_ARR("arr", arr, shown);
    BM_IN_U("len", len);
    BM_OUT_I("array_max(arr, len)", array_max(arr, len));
    BM_OUT_ARR("arr afterwards", arr, shown);
}

BM_DRIVER {
    int a[] = {3, -1, 7, 2};
    int one[] = {42};
    int neg[] = {-8, -3, -20, -5};
    int big[100];
    for (int i = 0; i < 100; i++) big[i] = i + 1;
    int dup[] = {5, 5, 5};
    int last[] = {1, 2, 3, 99};
    int first[] = {99, 2, 3, 1};
    int empty[1] = {123};

    sum_case("array_sum({3, -1, 7, 2}, 4) is 11", 0, a, 4, 4);
    sum_case("array_sum({42}, 1) is 42", 0, one, 1, 1);
    sum_case("array_sum of 1..100 is 5050", 0, big, 100, 100);
    sum_case("array_sum of all negatives", 0, neg, 4, 4);
    sum_case("array_sum with len 0 is 0 (reads nothing)", 0, empty, 1, 0);
    sum_case("array_sum only adds the first len elements", 0, a, 4, 2);

    max_case("array_max({3, -1, 7, 2}, 4) is 7", 0, a, 4, 4);
    max_case("array_max({42}, 1) is 42", 0, one, 1, 1);
    max_case("array_max of all negatives is -3", 0, neg, 4, 4);
    max_case("array_max finds the max in the last position", 0, last, 4, 4);
    max_case("array_max finds the max in the first position", 0, first, 4, 4);
    max_case("array_max({5, 5, 5}, 3) is 5", 0, dup, 3, 3);
    max_case("array_max only looks at the first len elements", 0, last, 4, 3);

    static const char *const sums[] = {"array_sum of a random array (1)", "array_sum of a random array (2)", "array_sum of a random array (3)", "array_sum of a random array (4)"};
    static const char *const maxes[] = {"array_max of a random array (1)", "array_max of a random array (2)", "array_max of a random array (3)", "array_max of a random array (4)"};
    int r[20];
    for (int k = 0; k < 4; k++) {
        size_t n = (size_t)bm_int(1, 20);
        bm_fill(r, n, -1000, 1000);
        sum_case(sums[k], 1, r, n, n);
    }
    for (int k = 0; k < 4; k++) {
        size_t n = (size_t)bm_int(1, 20);
        bm_fill(r, n, -1000, 1000);
        max_case(maxes[k], 1, r, n, n);
    }
    size_t n = (size_t)bm_int(2, 12);
    bm_fill(r, n, -1000, -1);
    max_case("array_max of a random all-negative array", 1, r, n, n);
    n = (size_t)bm_int(3, 20);
    bm_fill(r, n, -1000, 1000);
    sum_case("array_sum of a random prefix", 1, r, n, (size_t)bm_int(1, (int64_t)n - 1));
    n = (size_t)bm_int(3, 20);
    bm_fill(r, n, -1000, 1000);
    max_case("array_max of a random prefix", 1, r, n, (size_t)bm_int(1, (int64_t)n - 1));
}
