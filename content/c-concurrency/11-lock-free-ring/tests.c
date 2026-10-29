#include <sched.h>

#define ITEMS 100000
#define GIVE_UP 1000000L

typedef struct { Ring *r; int ok; int count; } Side;

static void *producer(void *arg) {
    Side *s = arg;
    for (int i = 0; i < ITEMS; i++) {
        long fails = 0;
        while (!ring_push(s->r, i)) {
            if (++fails > GIVE_UP) { s->ok = 0; return NULL; }
            sched_yield();
        }
    }
    return NULL;
}

static void *consumer(void *arg) {
    Side *s = arg;
    for (int i = 0; i < ITEMS; i++) {
        long fails = 0;
        int x = -1;
        while (!ring_pop(s->r, &x)) {
            if (++fails > GIVE_UP) { s->ok = 0; return NULL; }
            sched_yield();
        }
        if (x != i) s->ok = 0;
        s->count++;
    }
    return NULL;
}

int main(void) {
    Ring r;
    ring_init(&r);
    int x = -1;
    CHECK("popping an empty ring returns 0 and leaves the output alone", ring_pop(&r, &x) == 0 && x == -1, "returned a value");

    CHECK("push into an empty ring returns 1", ring_push(&r, 10) == 1, "expected 1");
    CHECK("pop returns the pushed value", ring_pop(&r, &x) == 1 && x == 10, "got %d", x);

    int pushed = 0;
    for (int i = 0; i < 20; i++) pushed += ring_push(&r, 100 + i);
    CHECK("the ring holds exactly RING_SIZE items and then reports full", pushed == RING_SIZE, "accepted %d items", pushed);

    int ok = 1, popped = 0;
    while (ring_pop(&r, &x)) { if (x != 100 + popped) ok = 0; popped++; }
    CHECK("items come out in the order they went in", ok && popped == RING_SIZE, "popped %d items", popped);

    int wrap_ok = 1;
    for (int round = 0; round < 50; round++) {
        for (int i = 0; i < 5; i++) if (!ring_push(&r, round * 10 + i)) wrap_ok = 0;
        for (int i = 0; i < 5; i++) if (!ring_pop(&r, &x) || x != round * 10 + i) wrap_ok = 0;
    }
    CHECK("the indices wrap around the array correctly", wrap_ok, "a value was lost or out of order");

    Ring q;
    ring_init(&q);
    Side ps = {&q, 1, 0}, cs = {&q, 1, 0};
    pthread_t a, b;
    pthread_create(&a, NULL, producer, &ps);
    pthread_create(&b, NULL, consumer, &cs);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    CHECK("a producer thread and a consumer thread exchange 100000 numbers in order", ps.ok && cs.ok && cs.count == ITEMS, "consumer got %d of %d numbers; producer ok %d, consumer ok %d", cs.count, ITEMS, ps.ok, cs.ok);
    return 0;
}
