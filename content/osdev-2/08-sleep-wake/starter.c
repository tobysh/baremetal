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
    /* TODO */
    (void)q;
}

int sleep_on(WaitQueue *q, Task *t) {
    /* TODO: reject non-READY tasks and full queues; append and block */
    (void)q;
    (void)t;
    return -1;
}

Task *wake_one(WaitQueue *q) {
    /* TODO: remove the oldest waiter, make it READY */
    (void)q;
    return NULL;
}
