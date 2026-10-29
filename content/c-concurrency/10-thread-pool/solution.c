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
    Pool *p = arg;
    for (;;) {
        pthread_mutex_lock(&p->lock);
        while (p->head == NULL && !p->shutdown) pthread_cond_wait(&p->has_work, &p->lock);
        if (p->head == NULL) {
            pthread_mutex_unlock(&p->lock);
            return NULL;
        }
        Task *t = p->head;
        p->head = t->next;
        if (p->head == NULL) p->tail = NULL;
        pthread_mutex_unlock(&p->lock);

        t->fn(t->arg);
        free(t);

        pthread_mutex_lock(&p->lock);
        p->pending--;
        if (p->pending == 0) pthread_cond_broadcast(&p->all_done);
        pthread_mutex_unlock(&p->lock);
    }
}

Pool *pool_create(int nthreads) {
    Pool *p = calloc(1, sizeof *p);
    pthread_mutex_init(&p->lock, NULL);
    pthread_cond_init(&p->has_work, NULL);
    pthread_cond_init(&p->all_done, NULL);
    p->nthreads = nthreads;
    for (int i = 0; i < nthreads; i++) pthread_create(&p->threads[i], NULL, pool_worker, p);
    return p;
}

void pool_submit(Pool *p, TaskFn fn, void *arg) {
    Task *t = malloc(sizeof *t);
    t->fn = fn;
    t->arg = arg;
    t->next = NULL;
    pthread_mutex_lock(&p->lock);
    if (p->tail) p->tail->next = t;
    else p->head = t;
    p->tail = t;
    p->pending++;
    pthread_cond_signal(&p->has_work);
    pthread_mutex_unlock(&p->lock);
}

void pool_wait(Pool *p) {
    pthread_mutex_lock(&p->lock);
    while (p->pending > 0) pthread_cond_wait(&p->all_done, &p->lock);
    pthread_mutex_unlock(&p->lock);
}

void pool_destroy(Pool *p) {
    pthread_mutex_lock(&p->lock);
    p->shutdown = 1;
    pthread_cond_broadcast(&p->has_work);
    pthread_mutex_unlock(&p->lock);
    for (int i = 0; i < p->nthreads; i++) pthread_join(p->threads[i], NULL);
    pthread_mutex_destroy(&p->lock);
    pthread_cond_destroy(&p->has_work);
    pthread_cond_destroy(&p->all_done);
    free(p);
}

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
