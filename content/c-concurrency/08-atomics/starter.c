#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

long atomic_count(int threads, int iters) {
    return -1;
}

void atomic_max(atomic_int *target, int v) {
}

void spin_lock(atomic_flag *f) {
}

void spin_unlock(atomic_flag *f) {
}

// Optional: try it with Run.
int main(void) {
    printf("count = %ld\n", atomic_count(4, 10000));
    atomic_int m = 5;
    atomic_max(&m, 9);
    atomic_max(&m, 3);
    printf("max = %d\n", atomic_load(&m));
    return 0;
}
