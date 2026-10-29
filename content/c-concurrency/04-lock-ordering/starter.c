#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

typedef struct {
    int id;
    pthread_mutex_t lock;
    long balance;
} Account;

void account_init(Account *a, int id, long balance) {
    a->id = id;
    a->balance = balance;
    pthread_mutex_init(&a->lock, NULL);
}

// Works on one thread, but freezes when two threads transfer in opposite directions.
int transfer(Account *from, Account *to, long amount) {
    pthread_mutex_lock(&from->lock);
    pthread_mutex_lock(&to->lock);
    int done = 0;
    if (from->balance >= amount) {
        from->balance -= amount;
        to->balance += amount;
        done = 1;
    }
    pthread_mutex_unlock(&to->lock);
    pthread_mutex_unlock(&from->lock);
    return done;
}

// Optional: try it with Run.
int main(void) {
    Account a, b;
    account_init(&a, 1, 100);
    account_init(&b, 2, 50);
    printf("transfer ok: %d\n", transfer(&a, &b, 30));
    printf("a = %ld, b = %ld\n", a.balance, b.balance);
    return 0;
}
