#include <limits.h>

void swap(int *a, int *b);

static void swap_pair(const char *name, int seeded, int x, int y) {
    if (seeded) BM_SEEDED(name); else BM_CASE(name);
    BM_IN_I("x", x);
    BM_IN_I("y", y);
    swap(&x, &y);
    BM_OUT_I("x", x);
    BM_OUT_I("y", y);
}

BM_DRIVER {
    swap_pair("swap(&x, &y) with 1 and 2", 0, 1, 2);
    swap_pair("swap handles negative numbers", 0, -7, 100);
    swap_pair("swap handles zero", 0, 0, 55);
    swap_pair("swapping equal values leaves both unchanged", 0, 9, 9);
    swap_pair("swap handles the extreme int values", 0, INT_MIN, INT_MAX);

    BM_CASE("swapping twice restores the original values");
    int p = 3, q = 4;
    BM_IN_I("p", p);
    BM_IN_I("q", q);
    swap(&p, &q);
    swap(&p, &q);
    BM_OUT_I("p", p);
    BM_OUT_I("q", q);

    BM_CASE("swap(&arr[0], &arr[2]) works on array elements");
    int arr[3] = {10, 20, 30};
    BM_IN_ARR("arr", arr, 3);
    swap(&arr[0], &arr[2]);
    BM_OUT_ARR("arr", arr, 3);

    static const char *const random_names[] = {
        "swap on random values (1)", "swap on random values (2)", "swap on random values (3)",
        "swap on random values (4)", "swap on random values (5)", "swap on random values (6)",
    };
    for (int i = 0; i < 6; i++) {
        int x = (int)bm_int(-100000, 100000);
        int y = (int)bm_int(-100000, 100000);
        swap_pair(random_names[i], 1, x, y);
    }

    static const char *const element_names[] = {
        "swap on two random elements of a random array (1)",
        "swap on two random elements of a random array (2)",
        "swap on two random elements of a random array (3)",
    };
    for (int k = 0; k < 3; k++) {
        int vals[8];
        size_t n = (size_t)bm_int(2, 8);
        bm_fill(vals, n, -999, 999);
        size_t i = (size_t)bm_int(0, (int64_t)n - 1);
        size_t j = (size_t)bm_int(0, (int64_t)n - 2);
        if (j >= i) j++;
        BM_SEEDED(element_names[k]);
        BM_IN_ARR("arr", vals, n);
        BM_IN_U("i", i);
        BM_IN_U("j", j);
        swap(&vals[i], &vals[j]);
        BM_OUT_ARR("arr", vals, n);
    }
}
