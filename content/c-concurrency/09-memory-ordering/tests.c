#include <sched.h>

#define BOXES 2000

typedef struct { Mailbox *boxes; } Sender;

static void *send_all(void *arg) {
    Sender *s = arg;
    for (int i = 0; i < BOXES; i++) {
        mailbox_send(&s->boxes[i], i, i * 2 + 1);
        if (i % 64 == 0) sched_yield();
    }
    return NULL;
}

typedef struct { atomic_flag *f; long *counter; int iters; } LockJob;
static void *lock_worker(void *arg) {
    LockJob *j = arg;
    for (int i = 0; i < j->iters; i++) {
        lock_acquire(j->f);
        long v = *j->counter;
        if ((i & 255) == 0) sched_yield();
        *j->counter = v + 1;
        lock_release(j->f);
    }
    return NULL;
}

int main(void) {
    Mailbox m = {0, 0, 0};
    int a = -1, b = -1;
    CHECK("an empty mailbox returns 0 and leaves the outputs alone", mailbox_try_receive(&m, &a, &b) == 0 && a == -1 && b == -1, "a = %d, b = %d", a, b);
    mailbox_send(&m, 3, 4);
    CHECK("mailbox_send makes the message visible", atomic_load(&m.ready) == 1, "ready is %d", atomic_load(&m.ready));
    CHECK("a full mailbox returns 1 and both values", mailbox_try_receive(&m, &a, &b) == 1 && a == 3 && b == 4, "a = %d, b = %d", a, b);

    Mailbox *boxes = calloc(BOXES, sizeof(Mailbox));
    Sender s = {boxes};
    pthread_t t;
    pthread_create(&t, NULL, send_all, &s);
    int ok = 1, seen = 0;
    long spins = 0;
    for (int i = 0; i < BOXES; i++) {
        int x = -1, y = -1;
        while (!mailbox_try_receive(&boxes[i], &x, &y)) {
            if (++spins > 100000000L) break;
            sched_yield();
        }
        if (x == i && y == i * 2 + 1) seen++;
        else ok = 0;
    }
    pthread_join(t, NULL);
    CHECK("a reader thread receives 2000 messages from a sender thread, all intact", ok && seen == BOXES, "%d of %d messages arrived correctly", seen, BOXES);
    free(boxes);

    atomic_flag f = ATOMIC_FLAG_INIT;
    lock_acquire(&f);
    int was_set = atomic_flag_test_and_set(&f);
    CHECK("lock_acquire leaves the flag set", was_set == 1, "the flag was not set after lock_acquire");
    lock_release(&f);
    int was_set2 = atomic_flag_test_and_set(&f);
    CHECK("lock_release clears the flag", was_set2 == 0, "the flag was still set after lock_release");
    atomic_flag_clear(&f);

    long counter = 0;
    LockJob lj = {&f, &counter, 20000};
    pthread_t w[4];
    for (int i = 0; i < 4; i++) pthread_create(&w[i], NULL, lock_worker, &lj);
    for (int i = 0; i < 4; i++) pthread_join(w[i], NULL);
    CHECK("the acquire/release spinlock protects a plain counter across four threads", counter == 80000, "expected 80000, got %ld", counter);
    return 0;
}
