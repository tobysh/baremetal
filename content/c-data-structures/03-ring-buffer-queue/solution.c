#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *buf;
    int cap;
    int head;
    int len;
} Queue;

int q_init(Queue *q, int cap) {
    q->buf = NULL;
    q->cap = 0;
    q->head = 0;
    q->len = 0;
    if (cap <= 0) return -1;
    q->buf = malloc((size_t)cap * sizeof *q->buf);
    if (q->buf == NULL) return -1;
    q->cap = cap;
    return 0;
}

int q_push(Queue *q, int x) {
    if (q->len == q->cap) return -1;
    q->buf[(q->head + q->len) % q->cap] = x;
    q->len++;
    return 0;
}

int q_pop(Queue *q, int *out) {
    if (q->len == 0) return -1;
    *out = q->buf[q->head];
    q->head = (q->head + 1) % q->cap;
    q->len--;
    return 0;
}

int q_peek(const Queue *q, int *out) {
    if (q->len == 0) return -1;
    *out = q->buf[q->head];
    return 0;
}

int q_size(const Queue *q) {
    return q->len;
}

void q_free(Queue *q) {
    free(q->buf);
    q->buf = NULL;
    q->len = 0;
}

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
