#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
    const int *a;
    int lo, hi;      // sum a[lo] .. a[hi - 1]
} Range;

void *sum_worker(void *arg) {
    Range *r = arg;
    long total = 0;
    for (int i = r->lo; i < r->hi; i++) total += r->a[i];
    return (void *)(intptr_t)total;
}

long parallel_sum(const int *a, int n, int nthreads) {
    Range ranges[8];
    pthread_t threads[8];
    int chunk = (n + nthreads - 1) / nthreads;
    for (int i = 0; i < nthreads; i++) {
        int lo = i * chunk, hi = lo + chunk;
        if (lo > n) lo = n;
        if (hi > n) hi = n;
        ranges[i] = (Range){a, lo, hi};
        pthread_create(&threads[i], NULL, sum_worker, &ranges[i]);
    }
    long sum = 0;
    for (int i = 0; i < nthreads; i++) {
        void *ret;
        pthread_join(threads[i], &ret);
        sum += (long)(intptr_t)ret;
    }
    return sum;
}

int main(void) {
    int data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    printf("sum = %ld\n", parallel_sum(data, 10, 3));
    return 0;
}
