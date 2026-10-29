#include <stdio.h>
#include <stddef.h>

static void swap(int *x, int *y) {
    int t = *x;
    *x = *y;
    *y = t;
}

size_t partition(int *a, size_t n) {
    int pivot = a[n - 1];
    size_t i = 0;
    for (size_t j = 0; j + 1 < n; j++) {
        if (a[j] <= pivot) {
            swap(&a[i], &a[j]);
            i++;
        }
    }
    swap(&a[i], &a[n - 1]);
    return i;
}

void quicksort(int *a, size_t n) {
    while (n >= 2) {
        size_t p = partition(a, n);
        if (p < n - p - 1) {
            quicksort(a, p);
            a += p + 1;
            n -= p + 1;
        } else {
            quicksort(a + p + 1, n - p - 1);
            n = p;
        }
    }
}

int quickselect(int *a, size_t n, size_t k) {
    for (;;) {
        size_t p = partition(a, n);
        if (p == k) return a[p];
        if (k < p) {
            n = p;
        } else {
            a += p + 1;
            n -= p + 1;
            k -= p + 1;
        }
    }
}

int main(void) {
    int a[] = {9, 4, 7, 1, 8, 2, 6};
    quicksort(a, 7);
    for (int i = 0; i < 7; i++) printf("%d ", a[i]);
    printf("\n");
    return 0;
}
