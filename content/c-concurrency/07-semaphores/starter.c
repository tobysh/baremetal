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
}

void limiter_enter(Limiter *l) {
}

void limiter_leave(Limiter *l) {
}

void limiter_destroy(Limiter *l) {
}

void *ping_worker(void *arg) {
    return NULL;
}

void *pong_worker(void *arg) {
    return NULL;
}

void ping_pong(int rounds, char *log) {
    log[0] = '\0';
}

// Optional: try it with Run.
int main(void) {
    char log[64];
    ping_pong(4, log);
    printf("%s\n", log);
    return 0;
}
