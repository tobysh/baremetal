#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*TaskFn)(void *);

typedef struct Task {
    TaskFn fn;
    void *arg;
    struct Task *next;
} Task;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t has_work;     // workers wait here for tasks
    pthread_cond_t all_done;     // pool_wait waits here for pending == 0
    Task *head, *tail;           // the queue of waiting tasks
    int pending;                 // tasks queued or running
    int shutdown;
    int nthreads;
    pthread_t threads[8];
} Pool;

void *pool_worker(void *arg) {
    return NULL;
}

Pool *pool_create(int nthreads) {
    Pool *p = calloc(1, sizeof *p);
    return p;
}

void pool_submit(Pool *p, TaskFn fn, void *arg) {
}

void pool_wait(Pool *p) {
}

void pool_destroy(Pool *p) {
    free(p);
}

// Optional: try it with Run.
static void hello(void *arg) {
    printf("task %d\n", *(int *)arg);
}

int main(void) {
    Pool *p = pool_create(2);
    int ids[3] = {1, 2, 3};
    for (int i = 0; i < 3; i++) pool_submit(p, hello, &ids[i]);
    pool_wait(p);
    pool_destroy(p);
    return 0;
}
