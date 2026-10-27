#include <stdint.h>
#include <stddef.h>

#define MAX_TASKS 8

typedef enum { TASK_READY, TASK_RUNNING, TASK_SLEEPING, TASK_BLOCKED, TASK_DEAD } TaskState;

typedef struct {
    int id;
    TaskState state;
    uint64_t wake_tick;
} Task;

void sched_init(Task *idle) {
}

int sched_add(Task *t) {
    return -1;
}

Task *sched_current(void) {
    return NULL;
}

Task *sched_pick_next(void) {
    return NULL;
}

void sched_sleep(uint64_t ticks) {
}

void sched_tick(void) {
}

uint64_t sched_now(void) {
    return 0;
}

void sched_block(void) {
}

void sched_unblock(Task *t) {
}

void sched_exit(void) {
}
