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
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

void counter_destroy(Counter *c) {
    pthread_mutex_destroy(&c->lock);
}

void counter_add(Counter *c, long delta) {
    pthread_mutex_lock(&c->lock);
    c->value += delta;
    pthread_mutex_unlock(&c->lock);
}

long counter_get(Counter *c) {
    pthread_mutex_lock(&c->lock);
    long v = c->value;
    pthread_mutex_unlock(&c->lock);
    return v;
}

int counter_add_if_below(Counter *c, long delta, long limit) {
    int added = 0;
    pthread_mutex_lock(&c->lock);
    if (c->value + delta <= limit) {
        c->value += delta;
        added = 1;
    }
    pthread_mutex_unlock(&c->lock);
    return added;
}

void *incr_worker(void *arg) {
    Job *job = arg;
    for (int i = 0; i < job->iters; i++) counter_add(job->counter, 1);
    return NULL;
}

long count_in_parallel(int threads, int iters) {
    Counter c;
    counter_init(&c);
    Job job = {&c, iters};
    pthread_t t[8];
    for (int i = 0; i < threads; i++) pthread_create(&t[i], NULL, incr_worker, &job);
    for (int i = 0; i < threads; i++) pthread_join(t[i], NULL);
    long result = counter_get(&c);
    counter_destroy(&c);
    return result;
}

int main(void) {
    printf("total = %ld (expected 40000)\n", count_in_parallel(4, 10000));
    return 0;
}
