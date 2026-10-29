#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_sorted(const int *a, int n) {
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

static int *sorted_copy(const int *a, int n) {
    int *c = malloc((size_t)n * sizeof *c);
    if (c == NULL) return NULL;
    memcpy(c, a, (size_t)n * sizeof *c);
    for (int i = 1; i < n; i++) {
        int key = c[i];
        int j = i - 1;
        while (j >= 0 && c[j] > key) {
            c[j + 1] = c[j];
            j--;
        }
        c[j + 1] = key;
    }
    return c;
}

int median_of(const int *a, int n) {
    int *c = sorted_copy(a, n);
    if (c == NULL) return 0;
    int m = c[(n - 1) / 2];
    free(c);
    return m;
}

static int lower_bound(const int *a, int n, int x) {
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] < x) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static int upper_bound(const int *a, int n, int x) {
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] <= x) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

int count_in_range(const int *a, int n, int lo, int hi) {
    if (lo > hi || n <= 0) return 0;
    int *c = sorted_copy(a, n);
    if (c == NULL) return -1;
    int r = upper_bound(c, n, hi) - lower_bound(c, n, lo);
    free(c);
    return r;
}

int main(void) {
    int a[] = {9, 1, 5, 3, 7};
    printf("sorted? %d (expected 0)\n", is_sorted(a, 5));
    printf("median %d (expected 5)\n", median_of(a, 5));
    printf("in [3,7]: %d (expected 3)\n", count_in_range(a, 5, 3, 7));
    return 0;
}
