#include <stdio.h>
#include <stddef.h>

static void swap(int *x, int *y) {
    int t = *x;
    *x = *y;
    *y = t;
}

size_t partition(int *a, size_t n) {
    return 0;
}

void quicksort(int *a, size_t n) {
}

int quickselect(int *a, size_t n, size_t k) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    int a[] = {9, 4, 7, 1, 8, 2, 6};
    quicksort(a, 7);
    for (int i = 0; i < 7; i++) printf("%d ", a[i]);
    printf("\n");
    return 0;
}
