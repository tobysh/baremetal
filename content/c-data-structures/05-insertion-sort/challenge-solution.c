#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long sort_count(int *a, int *tmp, int n) {
    if (n < 2) return 0;
    int mid = n / 2;
    long inv = sort_count(a, tmp, mid) + sort_count(a + mid, tmp, n - mid);
    int i = 0, j = mid, k = 0;
    while (i < mid && j < n) {
        if (a[j] < a[i]) {
            inv += mid - i;
            tmp[k++] = a[j++];
        } else {
            tmp[k++] = a[i++];
        }
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < n) tmp[k++] = a[j++];
    memcpy(a, tmp, (size_t)n * sizeof *a);
    return inv;
}

long count_inversions(const int *a, int n) {
    if (n < 2) return 0;
    int *copy = malloc((size_t)n * sizeof *copy);
    int *tmp = malloc((size_t)n * sizeof *tmp);
    memcpy(copy, a, (size_t)n * sizeof *copy);
    long inv = sort_count(copy, tmp, n);
    free(copy);
    free(tmp);
    return inv;
}

// Optional: try it with Run.
int main(void) {
    int a[] = {5, 2, 4, 6, 1, 3};
    printf("%ld inversions (expected 9)\n", count_inversions(a, 6));
    return 0;
}
