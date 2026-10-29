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
    return 0;
}

int ring_pop(Ring *r, int *out) {
    return 0;
}

// Optional: try it with Run.
int main(void) {
    Ring r;
    ring_init(&r);
    ring_push(&r, 5);
    ring_push(&r, 6);
    int x;
    while (ring_pop(&r, &x)) printf("popped %d\n", x);
    return 0;
}
