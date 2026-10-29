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
    m->a = a;
    m->b = b;
    atomic_store_explicit(&m->ready, 1, memory_order_release);
}

int mailbox_try_receive(Mailbox *m, int *a, int *b) {
    if (atomic_load_explicit(&m->ready, memory_order_acquire) == 0) return 0;
    *a = m->a;
    *b = m->b;
    return 1;
}

void lock_acquire(atomic_flag *f) {
    while (atomic_flag_test_and_set_explicit(f, memory_order_acquire)) {
    }
}

void lock_release(atomic_flag *f) {
    atomic_flag_clear_explicit(f, memory_order_release);
}

int main(void) {
    Mailbox m = {0, 0, 0};
    int a, b;
    printf("empty: %d\n", mailbox_try_receive(&m, &a, &b));
    mailbox_send(&m, 3, 4);
    printf("full: %d\n", mailbox_try_receive(&m, &a, &b));
    return 0;
}
