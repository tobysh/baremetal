#include <stdatomic.h>
#include <sched.h>
#include <unistd.h>
#include <malloc.h>

static void *watchdog(void *arg) {
    (void)arg;
    sleep(4);
    CHECK("finishes without hanging", 0, "still stuck after 4 seconds: check that pool_wait, pool_destroy and the workers always get woken");
    fflush(stdout);
    _exit(0);
    return NULL;
}

static atomic_long total;
static atomic_int on_main;
static pthread_t main_id;

static void add_task(void *arg) {
    atomic_fetch_add(&total, (long)(intptr_t)arg);
    if (pthread_equal(pthread_self(), main_id)) atomic_fetch_add(&on_main, 1);
}

static atomic_int started;
static atomic_int met;
static void rendezvous(void *arg) {
    (void)arg;
    atomic_fetch_add(&started, 1);
    long spins = 0;
    while (atomic_load(&started) < 3 && ++spins < 20000000L) sched_yield();
    if (atomic_load(&started) >= 3) atomic_fetch_add(&met, 1);
}

static Pool *shared_pool;
static void *submitter(void *arg) {
    long base = (long)(intptr_t)arg;
    for (int i = 1; i <= 500; i++) pool_submit(shared_pool, add_task, (void *)(intptr_t)(base + i));
    return NULL;
}

int main(void) {
    mallopt(M_ARENA_MAX, 1);   // the sandbox caps address space; extra malloc arenas would eat it
    main_id = pthread_self();
    pthread_t wd;
    pthread_create(&wd, NULL, watchdog, NULL);
    pthread_detach(wd);

    Pool *p = pool_create(4);
    CHECK("pool_create returns a pool", p != NULL, "got NULL");
    if (!p) return 0;
    pool_wait(p);
    CHECK("pool_wait returns at once when nothing was submitted", 1, "");

    for (int i = 1; i <= 1000; i++) pool_submit(p, add_task, (void *)(intptr_t)i);
    pool_wait(p);
    CHECK("pool_wait returns after 1000 tasks have all run", atomic_load(&total) == 500500, "total %ld (expected 500500)", atomic_load(&total));
    CHECK("tasks run on worker threads, not on the caller", atomic_load(&on_main) == 0, "%d tasks ran on the calling thread", atomic_load(&on_main));

    atomic_store(&total, 0);
    for (int i = 1; i <= 200; i++) pool_submit(p, add_task, (void *)(intptr_t)(i * 2));
    pool_wait(p);
    CHECK("the pool can be reused after pool_wait", atomic_load(&total) == 40200, "total %ld (expected 40200)", atomic_load(&total));
    pool_destroy(p);

    Pool *three = pool_create(3);
    for (int i = 0; i < 3; i++) pool_submit(three, rendezvous, NULL);
    pool_wait(three);
    CHECK("three workers run three tasks at the same time", atomic_load(&met) == 3, "only %d tasks saw all three running together", atomic_load(&met));
    pool_destroy(three);

    atomic_store(&total, 0);
    shared_pool = pool_create(4);
    pthread_t s[3];
    for (int i = 0; i < 3; i++) pthread_create(&s[i], NULL, submitter, (void *)(intptr_t)(i * 1000));
    for (int i = 0; i < 3; i++) pthread_join(s[i], NULL);
    pool_wait(shared_pool);
    long want = 0;
    for (int i = 0; i < 3; i++) for (int k = 1; k <= 500; k++) want += i * 1000 + k;
    CHECK("three threads can submit at the same time without losing tasks", atomic_load(&total) == want, "total %ld (expected %ld)", atomic_load(&total), want);
    pool_destroy(shared_pool);

    atomic_store(&total, 0);
    Pool *q = pool_create(2);
    for (int i = 0; i < 300; i++) pool_submit(q, add_task, (void *)(intptr_t)1);
    pool_destroy(q);
    CHECK("pool_destroy finishes the tasks that were already submitted", atomic_load(&total) == 300, "only %ld of 300 tasks ran", atomic_load(&total));

    Pool *one = pool_create(1);
    atomic_store(&total, 0);
    for (int i = 1; i <= 100; i++) pool_submit(one, add_task, (void *)(intptr_t)i);
    pool_wait(one);
    CHECK("a pool of one worker also works", atomic_load(&total) == 5050, "total %ld", atomic_load(&total));
    pool_destroy(one);
    return 0;
}
