#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_sorted(const int *a, int n) {
    return 0;
}

int median_of(const int *a, int n) {
    return 0;
}

int count_in_range(const int *a, int n, int lo, int hi) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    int a[] = {9, 1, 5, 3, 7};
    printf("sorted? %d (expected 0)\n", is_sorted(a, 5));
    printf("median %d (expected 5)\n", median_of(a, 5));
    printf("in [3,7]: %d (expected 3)\n", count_in_range(a, 5, 3, 7));
    return 0;
}
