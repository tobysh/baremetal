#include <stddef.h>

enum { READY, BLOCKED };

typedef struct {
    int id;
    int state;    /* READY or BLOCKED */
} Task;

#define QCAP 8

/* A FIFO ring buffer: the oldest waiter is items[head], the next ones follow
   at (head + 1) % QCAP, (head + 2) % QCAP, ... for count entries. */
typedef struct {
    Task *items[QCAP];
    unsigned head;
    unsigned count;
} WaitQueue;

void wait_init(WaitQueue *q) {
    q->head = 0;
    q->count = 0;
}

int sleep_on(WaitQueue *q, Task *t) {
    if (t->state != READY || q->count == QCAP)
        return -1;                          /* nothing changes on failure */
    q->items[(q->head + q->count) % QCAP] = t;
    q->count++;
    t->state = BLOCKED;
    return 0;
}

Task *wake_one(WaitQueue *q) {
    if (q->count == 0)
        return NULL;                        /* waking nobody is harmless */
    Task *t = q->items[q->head];
    q->head = (q->head + 1) % QCAP;
    q->count--;
    t->state = READY;
    return t;
}
