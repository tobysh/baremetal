#include <unistd.h>

static void *watchdog(void *arg) {
    (void)arg;
    sleep(3);
    CHECK("finishes without freezing", 0, "the threads were still stuck after 3 seconds: that is a deadlock");
    fflush(stdout);
    _exit(0);
    return NULL;
}

typedef struct { Account *acc; int count; unsigned seed; int dir; long moved; } Mover;

static void *opposite(void *arg) {
    Mover *m = arg;
    for (int i = 0; i < m->count; i++) {
        if (m->dir == 0) m->moved += transfer(&m->acc[0], &m->acc[1], 1);
        else m->moved += transfer(&m->acc[1], &m->acc[0], 1);
    }
    return NULL;
}

static void *shuffle(void *arg) {
    Mover *m = arg;
    unsigned s = m->seed;
    for (int i = 0; i < m->count; i++) {
        s = s * 1103515245u + 12345u;
        int a = (int)((s >> 16) % 4);
        s = s * 1103515245u + 12345u;
        int b = (int)((s >> 16) % 4);
        s = s * 1103515245u + 12345u;
        long amt = (long)((s >> 16) % 7);
        transfer(&m->acc[a], &m->acc[b], amt);
    }
    return NULL;
}

int main(void) {
    pthread_t wd;
    pthread_create(&wd, NULL, watchdog, NULL);
    pthread_detach(wd);

    Account acc[4];
    for (int i = 0; i < 4; i++) account_init(&acc[i], i + 1, 100);

    CHECK("transfer moves money and returns 1", transfer(&acc[0], &acc[1], 30) == 1 && acc[0].balance == 70 && acc[1].balance == 130, "balances %ld and %ld", acc[0].balance, acc[1].balance);
    CHECK("transfer refuses more than the balance", transfer(&acc[0], &acc[1], 71) == 0 && acc[0].balance == 70 && acc[1].balance == 130, "balances %ld and %ld", acc[0].balance, acc[1].balance);
    CHECK("transfer can empty an account exactly", transfer(&acc[0], &acc[1], 70) == 1 && acc[0].balance == 0, "balance %ld", acc[0].balance);
    CHECK("transfer refuses zero and negative amounts", transfer(&acc[1], &acc[2], 0) == 0 && transfer(&acc[1], &acc[2], -5) == 0 && acc[1].balance == 200 && acc[2].balance == 100, "balances %ld and %ld", acc[1].balance, acc[2].balance);
    CHECK("transfer to the same account does nothing and does not freeze", transfer(&acc[3], &acc[3], 10) == 0 && acc[3].balance == 100, "balance %ld", acc[3].balance);

    Account ab[2];
    account_init(&ab[0], 1, 1000);
    account_init(&ab[1], 2, 1000);
    Mover m[2] = {{ab, 100000, 0, 0, 0}, {ab, 100000, 0, 1, 0}};
    pthread_t t[4];
    pthread_create(&t[0], NULL, opposite, &m[0]);
    pthread_create(&t[1], NULL, opposite, &m[1]);
    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    long total = ab[0].balance + ab[1].balance;
    CHECK("two threads transferring in opposite directions finish and keep the total", total == 2000 && ab[0].balance >= 0 && ab[1].balance >= 0, "total %ld (expected 2000), balances %ld and %ld", total, ab[0].balance, ab[1].balance);

    for (int i = 0; i < 4; i++) { acc[i].balance = 100; }
    Mover s[4];
    for (int i = 0; i < 4; i++) s[i] = (Mover){acc, 50000, 7u + (unsigned)i * 31u, 0, 0};
    for (int i = 0; i < 4; i++) pthread_create(&t[i], NULL, shuffle, &s[i]);
    for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);
    long all = 0;
    int nonneg = 1;
    for (int i = 0; i < 4; i++) { all += acc[i].balance; if (acc[i].balance < 0) nonneg = 0; }
    CHECK("four threads moving money between four accounts keep the total of 400", all == 400 && nonneg, "total %ld, nonnegative %d", all, nonneg);
    return 0;
}
