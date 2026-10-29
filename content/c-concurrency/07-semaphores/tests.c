#include <sched.h>
#include <unistd.h>

static void *watchdog(void *arg) {
    (void)arg;
    sleep(3);
    CHECK("finishes without hanging", 0, "threads were still waiting after 3 seconds: a sem_post is missing or a semaphore starts at the wrong value");
    fflush(stdout);
    _exit(0);
    return NULL;
}

typedef struct { Limiter *l; pthread_mutex_t m; int active; int peak; int done; } Gate;

static void *visitor(void *arg) {
    Gate *g = arg;
    limiter_enter(g->l);
    pthread_mutex_lock(&g->m);
    g->active++;
    if (g->active > g->peak) g->peak = g->active;
    pthread_mutex_unlock(&g->m);
    for (int i = 0; i < 300; i++) sched_yield();
    pthread_mutex_lock(&g->m);
    g->active--;
    g->done++;
    pthread_mutex_unlock(&g->m);
    limiter_leave(g->l);
    return NULL;
}

static int run_gate(int visitors, int limit, int *peak, int *done) {
    Limiter l;
    limiter_init(&l, limit);
    Gate g = {&l, PTHREAD_MUTEX_INITIALIZER, 0, 0, 0};
    pthread_t t[8];
    for (int i = 0; i < visitors; i++) pthread_create(&t[i], NULL, visitor, &g);
    for (int i = 0; i < visitors; i++) pthread_join(t[i], NULL);
    limiter_destroy(&l);
    *peak = g.peak;
    *done = g.done;
    return g.active;
}

int main(void) {
    pthread_t wd;
    pthread_create(&wd, NULL, watchdog, NULL);
    pthread_detach(wd);

    char log[2100];
    ping_pong(3, log);
    CHECK("three rounds of ping-pong are PpPpPp", strcmp(log, "PpPpPp") == 0, "got \"%s\"", log);
    ping_pong(1, log);
    CHECK("one round is Pp", strcmp(log, "Pp") == 0, "got \"%s\"", log);
    ping_pong(0, log);
    CHECK("zero rounds gives an empty string", strcmp(log, "") == 0, "got \"%s\"", log);

    int good = 1;
    for (int rep = 0; rep < 20; rep++) {
        ping_pong(1000, log);
        if (strlen(log) != 2000) { good = 0; break; }
        for (int i = 0; i < 2000; i++) if (log[i] != (i % 2 == 0 ? 'P' : 'p')) { good = 0; break; }
        if (!good) break;
    }
    CHECK("1000 rounds alternate perfectly, 20 times in a row", good, "the log was not strictly PpPp...");

    int peak = 0, done = 0, active;
    active = run_gate(8, 1, &peak, &done);
    CHECK("a limiter with limit 1 lets one thread in at a time", peak == 1 && done == 8 && active == 0, "peak %d, finished %d, still inside %d", peak, done, active);
    active = run_gate(8, 3, &peak, &done);
    CHECK("a limiter with limit 3 never lets more than 3 in", peak >= 1 && peak <= 3 && done == 8 && active == 0, "peak %d, finished %d, still inside %d", peak, done, active);
    active = run_gate(5, 8, &peak, &done);
    CHECK("a limit larger than the number of threads blocks nobody", done == 5 && active == 0, "finished %d", done);
    return 0;
}
