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
    return NULL;
}

void parallel_histogram(const unsigned char *data, size_t n, int nthreads, long hist[256]) {
}

// Optional: try it with Run.
int main(void) {
    unsigned char text[] = "hello threads";
    long hist[256];
    parallel_histogram(text, sizeof text - 1, 3, hist);
    printf("'l' appears %ld times\n", hist['l']);
    return 0;
}
