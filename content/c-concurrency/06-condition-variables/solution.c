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
    pthread_mutex_lock(&b->lock);
    while (b->count == BB_CAP) pthread_cond_wait(&b->not_full, &b->lock);
    b->items[(b->head + b->count) % BB_CAP] = item;
    b->count++;
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->lock);
}

int bb_get(BoundedBuffer *b, int *out) {
    pthread_mutex_lock(&b->lock);
    while (b->count == 0 && !b->closed) pthread_cond_wait(&b->not_empty, &b->lock);
    if (b->count == 0) {
        pthread_mutex_unlock(&b->lock);
        return 0;
    }
    *out = b->items[b->head];
    b->head = (b->head + 1) % BB_CAP;
    b->count--;
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->lock);
    return 1;
}

void bb_close(BoundedBuffer *b) {
    pthread_mutex_lock(&b->lock);
    b->closed = 1;
    pthread_cond_broadcast(&b->not_empty);
    pthread_cond_broadcast(&b->not_full);
    pthread_mutex_unlock(&b->lock);
}

int main(void) {
    BoundedBuffer b;
    bb_init(&b);
    bb_put(&b, 10);
    bb_put(&b, 20);
    int x;
    while (b.count > 0 && bb_get(&b, &x)) printf("got %d\n", x);
    return 0;
}
