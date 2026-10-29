#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

typedef struct {
    atomic_long *counter;
    int iters;
} CountJob;

static void *count_worker(void *arg) {
    CountJob *job = arg;
    for (int i = 0; i < job->iters; i++) atomic_fetch_add(job->counter, 1);
    return NULL;
}

long atomic_count(int threads, int iters) {
    atomic_long c = 0;
    CountJob job = {&c, iters};
    pthread_t t[8];
    for (int i = 0; i < threads; i++) pthread_create(&t[i], NULL, count_worker, &job);
    for (int i = 0; i < threads; i++) pthread_join(t[i], NULL);
    return atomic_load(&c);
}

void atomic_max(atomic_int *target, int v) {
    int old = atomic_load(target);
    while (old < v && !atomic_compare_exchange_weak(target, &old, v)) {
    }
}

void spin_lock(atomic_flag *f) {
    while (atomic_flag_test_and_set(f)) {
    }
}

void spin_unlock(atomic_flag *f) {
    atomic_flag_clear(f);
}

int main(void) {
    printf("count = %ld\n", atomic_count(4, 10000));
    atomic_int m = 5;
    atomic_max(&m, 9);
    atomic_max(&m, 3);
    printf("max = %d\n", atomic_load(&m));
    return 0;
}
