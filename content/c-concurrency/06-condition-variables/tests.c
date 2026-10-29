#include <unistd.h>
#include <sched.h>

static void *watchdog(void *arg) {
    (void)arg;
    sleep(3);
    CHECK("finishes without hanging", 0, "threads were still waiting after 3 seconds: a wakeup is missing, or a thread waits when it should not");
    fflush(stdout);
    _exit(0);
    return NULL;
}

#define PER 3000
typedef struct { BoundedBuffer *b; int id; } Prod;
typedef struct { BoundedBuffer *b; long sum; long count; int order_ok; } Cons;

static void *producer(void *arg) {
    Prod *p = arg;
    for (int i = 0; i < PER; i++) bb_put(p->b, p->id * 100000 + i);
    return NULL;
}

static void *consumer(void *arg) {
    Cons *c = arg;
    int last[8], x;
    for (int i = 0; i < 8; i++) last[i] = -1;
    while (bb_get(c->b, &x)) {
        int id = x / 100000, seq = x % 100000;
        if (id < 0 || id >= 8 || seq <= last[id]) c->order_ok = 0;
        else last[id] = seq;
        c->sum += x;
        c->count++;
    }
    return NULL;
}

static void *late_getter(void *arg) {
    Cons *c = arg;
    int x = -1;
    c->count = bb_get(c->b, &x);
    c->sum = x;
    return NULL;
}

static void *flood(void *arg) {
    BoundedBuffer *b = arg;
    for (int i = 0; i < 20; i++) bb_put(b, i);
    return NULL;
}

int main(void) {
    pthread_t wd;
    pthread_create(&wd, NULL, watchdog, NULL);
    pthread_detach(wd);

    BoundedBuffer b;
    bb_init(&b);
    int x = -1, ok = 1;
    for (int i = 1; i <= 3; i++) bb_put(&b, i * 11);
    for (int i = 1; i <= 3; i++) { if (!bb_get(&b, &x) || x != i * 11) ok = 0; }
    CHECK("items come out in the order they went in", ok, "last item was %d", x);

    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 4; i++) bb_put(&b, round * 10 + i);
        for (int i = 0; i < 4; i++) { if (!bb_get(&b, &x) || x != round * 10 + i) ok = 0; }
    }
    CHECK("the ring wraps around correctly when filled to capacity", ok && b.count == 0, "count %d", b.count);

    bb_put(&b, 7);
    bb_put(&b, 8);
    bb_close(&b);
    int g1 = bb_get(&b, &x), v1 = x;
    int g2 = bb_get(&b, &x), v2 = x;
    int g3 = bb_get(&b, &x);
    CHECK("a closed buffer is drained first, then bb_get returns 0", g1 == 1 && v1 == 7 && g2 == 1 && v2 == 8 && g3 == 0, "results %d/%d, %d/%d, %d", g1, v1, g2, v2, g3);

    BoundedBuffer lb;
    bb_init(&lb);
    Cons late = {&lb, 0, -1, 1};
    pthread_t t[8];
    pthread_create(&t[0], NULL, late_getter, &late);
    for (int i = 0; i < 2000; i++) sched_yield();
    bb_put(&lb, 99);
    pthread_join(t[0], NULL);
    CHECK("a consumer that is already waiting gets an item put later", late.count == 1 && late.sum == 99, "returned %ld, item %ld", late.count, late.sum);

    BoundedBuffer cb;
    bb_init(&cb);
    Cons idle = {&cb, 0, -1, 1};
    pthread_create(&t[0], NULL, late_getter, &idle);
    for (int i = 0; i < 2000; i++) sched_yield();
    bb_close(&cb);
    pthread_join(t[0], NULL);
    CHECK("bb_close wakes a consumer that is waiting on an empty buffer", idle.count == 0, "bb_get returned %ld", idle.count);

    BoundedBuffer fb;
    bb_init(&fb);
    pthread_create(&t[0], NULL, flood, &fb);
    for (int i = 0; i < 2000; i++) sched_yield();
    int in_order = 1;
    for (int i = 0; i < 20; i++) { if (!bb_get(&fb, &x) || x != i) in_order = 0; }
    pthread_join(t[0], NULL);
    CHECK("a producer that outruns the buffer waits and loses nothing", in_order && fb.count == 0, "count %d", fb.count);

    BoundedBuffer sb;
    bb_init(&sb);
    Prod prods[3];
    Cons cons[3];
    for (int i = 0; i < 3; i++) { prods[i] = (Prod){&sb, i}; cons[i] = (Cons){&sb, 0, 0, 1}; }
    for (int i = 0; i < 3; i++) pthread_create(&t[i], NULL, producer, &prods[i]);
    for (int i = 0; i < 3; i++) pthread_create(&t[3 + i], NULL, consumer, &cons[i]);
    for (int i = 0; i < 3; i++) pthread_join(t[i], NULL);
    bb_close(&sb);
    for (int i = 0; i < 3; i++) pthread_join(t[3 + i], NULL);
    long sum = 0, count = 0;
    for (int i = 0; i < 3; i++) { sum += cons[i].sum; count += cons[i].count; }
    long want = 0;
    for (int id = 0; id < 3; id++) for (int i = 0; i < PER; i++) want += id * 100000 + i;
    CHECK("three producers and three consumers move every item exactly once", count == 3 * PER && sum == want, "got %ld items with sum %ld, expected %d items with sum %ld", count, sum, 3 * PER, want);
    CHECK("each consumer sees each producer's items in order", cons[0].order_ok && cons[1].order_ok && cons[2].order_ok, "items arrived out of order");
    return 0;
}
