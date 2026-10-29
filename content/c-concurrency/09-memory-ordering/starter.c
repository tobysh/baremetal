#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

typedef struct {
    int a;
    int b;
    atomic_int ready;     // 0 = empty, 1 = full
} Mailbox;

void mailbox_send(Mailbox *m, int a, int b) {
}

int mailbox_try_receive(Mailbox *m, int *a, int *b) {
    return 0;
}

void lock_acquire(atomic_flag *f) {
}

void lock_release(atomic_flag *f) {
}

// Optional: try it with Run.
int main(void) {
    Mailbox m = {0, 0, 0};
    int a, b;
    printf("empty: %d\n", mailbox_try_receive(&m, &a, &b));
    mailbox_send(&m, 3, 4);
    printf("full: %d\n", mailbox_try_receive(&m, &a, &b));
    return 0;
}
