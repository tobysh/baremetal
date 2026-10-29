#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

typedef struct {
    sem_t slots;
} Limiter;

typedef struct {
    sem_t turn_ping;
    sem_t turn_pong;
    char *log;
    int pos;
    int rounds;
} PingPong;

void limiter_init(Limiter *l, int limit) {
    sem_init(&l->slots, 0, (unsigned)limit);
}

void limiter_enter(Limiter *l) {
    sem_wait(&l->slots);
}

void limiter_leave(Limiter *l) {
    sem_post(&l->slots);
}

void limiter_destroy(Limiter *l) {
    sem_destroy(&l->slots);
}

void *ping_worker(void *arg) {
    PingPong *pp = arg;
    for (int r = 0; r < pp->rounds; r++) {
        sem_wait(&pp->turn_ping);
        pp->log[pp->pos++] = 'P';
        sem_post(&pp->turn_pong);
    }
    return NULL;
}

void *pong_worker(void *arg) {
    PingPong *pp = arg;
    for (int r = 0; r < pp->rounds; r++) {
        sem_wait(&pp->turn_pong);
        pp->log[pp->pos++] = 'p';
        sem_post(&pp->turn_ping);
    }
    return NULL;
}

void ping_pong(int rounds, char *log) {
    PingPong pp;
    sem_init(&pp.turn_ping, 0, 1);
    sem_init(&pp.turn_pong, 0, 0);
    pp.log = log;
    pp.pos = 0;
    pp.rounds = rounds;
    pthread_t a, b;
    pthread_create(&a, NULL, ping_worker, &pp);
    pthread_create(&b, NULL, pong_worker, &pp);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    log[pp.pos] = '\0';
    sem_destroy(&pp.turn_ping);
    sem_destroy(&pp.turn_pong);
}

int main(void) {
    char log[64];
    ping_pong(4, log);
    printf("%s\n", log);
    return 0;
}
