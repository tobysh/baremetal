#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>

#define RING_SIZE 8

typedef struct {
    int buf[RING_SIZE];
    atomic_size_t head;    // items popped so far (written by the consumer only)
    atomic_size_t tail;    // items pushed so far (written by the producer only)
} Ring;

void ring_init(Ring *r) {
    atomic_init(&r->head, 0);
    atomic_init(&r->tail, 0);
}

int ring_push(Ring *r, int value) {
    size_t t = atomic_load_explicit(&r->tail, memory_order_relaxed);
    size_t h = atomic_load_explicit(&r->head, memory_order_acquire);
    if (t - h == RING_SIZE) return 0;
    r->buf[t % RING_SIZE] = value;
    atomic_store_explicit(&r->tail, t + 1, memory_order_release);
    return 1;
}

int ring_pop(Ring *r, int *out) {
    size_t h = atomic_load_explicit(&r->head, memory_order_relaxed);
    size_t t = atomic_load_explicit(&r->tail, memory_order_acquire);
    if (h == t) return 0;
    *out = r->buf[h % RING_SIZE];
    atomic_store_explicit(&r->head, h + 1, memory_order_release);
    return 1;
}

int main(void) {
    Ring r;
    ring_init(&r);
    ring_push(&r, 5);
    ring_push(&r, 6);
    int x;
    while (ring_pop(&r, &x)) printf("popped %d\n", x);
    return 0;
}
