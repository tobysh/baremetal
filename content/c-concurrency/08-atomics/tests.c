#include <sched.h>

typedef struct { atomic_int *m; int base; int n; } MaxJob;
static void *max_worker(void *arg) {
    MaxJob *j = arg;
    for (int i = 0; i < j->n; i++) atomic_max(j->m, j->base + i);
    return NULL;
}

typedef struct { atomic_flag *f; long *counter; int iters; } LockJob;
static void *lock_worker(void *arg) {
    LockJob *j = arg;
    for (int i = 0; i < j->iters; i++) {
        spin_lock(j->f);
        long v = *j->counter;
        if ((i & 255) == 0) sched_yield();
        *j->counter = v + 1;
        spin_unlock(j->f);
    }
    return NULL;
}

int main(void) {
    int ok = 1;
    long got = 0;
    for (int rep = 0; rep < 5; rep++) {
        got = atomic_count(4, 50000);
        if (got != 200000) ok = 0;
    }
    CHECK("atomic_count(4, 50000) is exactly 200000 every time", ok, "last result was %ld", got);
    CHECK("atomic_count with one thread", atomic_count(1, 777) == 777, "got %ld", atomic_count(1, 777));
    CHECK("atomic_count with zero iterations", atomic_count(5, 0) == 0, "got %ld", atomic_count(5, 0));

    atomic_int m = 5;
    atomic_max(&m, 9);
    CHECK("atomic_max raises the value", atomic_load(&m) == 9, "got %d", atomic_load(&m));
    atomic_max(&m, 3);
    CHECK("atomic_max ignores a smaller value", atomic_load(&m) == 9, "got %d", atomic_load(&m));
    atomic_max(&m, 9);
    CHECK("atomic_max with an equal value changes nothing", atomic_load(&m) == 9, "got %d", atomic_load(&m));
    atomic_int neg = -50;
    atomic_max(&neg, -20);
    CHECK("atomic_max works with negative numbers", atomic_load(&neg) == -20, "got %d", atomic_load(&neg));

    atomic_int hi = 0;
    MaxJob mj[4];
    pthread_t t[4];
    for (int i = 0; i < 4; i++) mj[i] = (MaxJob){&hi, i * 1000, 20000};
    for (int i = 0; i < 4; i++) pthread_create(&t[i], NULL, max_worker, &mj[i]);
    for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);
    CHECK("four threads racing atomic_max end on the true maximum", atomic_load(&hi) == 3000 + 19999, "expected %d, got %d", 3000 + 19999, atomic_load(&hi));

    atomic_flag f = ATOMIC_FLAG_INIT;
    spin_lock(&f);
    int was_set = atomic_flag_test_and_set(&f);
    CHECK("spin_lock leaves the flag set", was_set == 1, "the flag was not set after spin_lock");
    spin_unlock(&f);
    int was_set2 = atomic_flag_test_and_set(&f);
    CHECK("spin_unlock clears the flag", was_set2 == 0, "the flag was still set after spin_unlock");
    atomic_flag_clear(&f);

    long counter = 0;
    LockJob lj = {&f, &counter, 20000};
    for (int i = 0; i < 4; i++) pthread_create(&t[i], NULL, lock_worker, &lj);
    for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);
    CHECK("a spinlock protects a plain counter across four threads", counter == 80000, "expected 80000, got %ld", counter);
    return 0;
}
