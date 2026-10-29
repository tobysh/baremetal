#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *buf;
    int cap;
    int head;
    int len;
} Queue;

int q_init(Queue *q, int cap) {
    return -1;
}

int q_push(Queue *q, int x) {
    return -1;
}

int q_pop(Queue *q, int *out) {
    return -1;
}

int q_peek(const Queue *q, int *out) {
    return -1;
}

int q_size(const Queue *q) {
    return 0;
}

void q_free(Queue *q) {
}

// Optional: try it with Run.
int main(void) {
    Queue q;
    q_init(&q, 3);
    q_push(&q, 1);
    q_push(&q, 2);
    int x;
    q_pop(&q, &x);
    printf("popped %d (expected 1), size %d\n", x, q_size(&q));
    q_free(&q);
    return 0;
}
