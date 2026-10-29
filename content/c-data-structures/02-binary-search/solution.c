#include <stdio.h>

int lower_bound(const int *a, int n, int x) {
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] < x) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

int upper_bound(const int *a, int n, int x) {
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] <= x) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

int count_equal(const int *a, int n, int x) {
    return upper_bound(a, n, x) - lower_bound(a, n, x);
}

int main(void) {
    int a[] = {1, 3, 3, 3, 8, 12};
    printf("lower_bound(3) = %d (expected 1)\n", lower_bound(a, 6, 3));
    printf("upper_bound(3) = %d (expected 4)\n", upper_bound(a, 6, 3));
    return 0;
}
