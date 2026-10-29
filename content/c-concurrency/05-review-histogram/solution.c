#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    const unsigned char *data;
    size_t lo, hi;
    long *shared;
    pthread_mutex_t *lock;
} Task;

void *hist_worker(void *arg) {
    Task *t = arg;
    long local[256] = {0};
    for (size_t i = t->lo; i < t->hi; i++) local[t->data[i]]++;
    pthread_mutex_lock(t->lock);
    for (int b = 0; b < 256; b++) t->shared[b] += local[b];
    pthread_mutex_unlock(t->lock);
    return NULL;
}

void parallel_histogram(const unsigned char *data, size_t n, int nthreads, long hist[256]) {
    for (int b = 0; b < 256; b++) hist[b] = 0;
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);
    Task tasks[8];
    pthread_t threads[8];
    size_t chunk = (n + (size_t)nthreads - 1) / (size_t)nthreads;
    for (int i = 0; i < nthreads; i++) {
        size_t lo = (size_t)i * chunk, hi = lo + chunk;
        if (lo > n) lo = n;
        if (hi > n) hi = n;
        tasks[i] = (Task){data, lo, hi, hist, &lock};
        pthread_create(&threads[i], NULL, hist_worker, &tasks[i]);
    }
    for (int i = 0; i < nthreads; i++) pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&lock);
}

int main(void) {
    unsigned char text[] = "hello threads";
    long hist[256];
    parallel_histogram(text, sizeof text - 1, 3, hist);
    printf("'l' appears %ld times\n", hist['l']);
    return 0;
}
