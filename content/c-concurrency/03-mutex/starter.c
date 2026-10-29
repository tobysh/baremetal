#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

typedef struct {
    pthread_mutex_t lock;
    long value;
} Counter;

typedef struct {
    Counter *counter;
    int iters;
} Job;

void counter_init(Counter *c) {
}

void counter_destroy(Counter *c) {
}

void counter_add(Counter *c, long delta) {
}

long counter_get(Counter *c) {
    return -1;
}

int counter_add_if_below(Counter *c, long delta, long limit) {
    return 0;
}

void *incr_worker(void *arg) {
    return NULL;
}

long count_in_parallel(int threads, int iters) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    printf("total = %ld (expected 40000)\n", count_in_parallel(4, 10000));
    return 0;
}
