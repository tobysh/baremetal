#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
    const int *a;
    int lo, hi;      // sum a[lo] .. a[hi - 1]
} Range;

void *sum_worker(void *arg) {
    return NULL;
}

long parallel_sum(const int *a, int n, int nthreads) {
    return 0;
}

// Optional: try it with Run.
int main(void) {
    int data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    printf("sum = %ld\n", parallel_sum(data, 10, 3));
    return 0;
}
