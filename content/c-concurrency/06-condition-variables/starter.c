#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

#define BB_CAP 4

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int items[BB_CAP];
    int head;      // index of the oldest item
    int count;     // how many items are stored
    int closed;
} BoundedBuffer;

void bb_init(BoundedBuffer *b) {
    pthread_mutex_init(&b->lock, NULL);
    pthread_cond_init(&b->not_empty, NULL);
    pthread_cond_init(&b->not_full, NULL);
    b->head = 0;
    b->count = 0;
    b->closed = 0;
}

void bb_put(BoundedBuffer *b, int item) {
}

int bb_get(BoundedBuffer *b, int *out) {
    return 0;
}

void bb_close(BoundedBuffer *b) {
}

// Optional: try it with Run.
int main(void) {
    BoundedBuffer b;
    bb_init(&b);
    bb_put(&b, 10);
    bb_put(&b, 20);
    int x;
    while (b.count > 0 && bb_get(&b, &x)) printf("got %d\n", x);
    return 0;
}
