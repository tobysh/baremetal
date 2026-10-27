#include <stdint.h>
#include <stddef.h>

#define MAX_TASKS 8

typedef enum { TASK_READY, TASK_RUNNING, TASK_SLEEPING, TASK_BLOCKED, TASK_DEAD } TaskState;

typedef struct {
    int id;
    TaskState state;
    uint64_t wake_tick;
} Task;

static Task *slots[MAX_TASKS];
static Task *idle_task;
static Task *current;
static int current_slot = -1;   // -1 while the idle task is current
static uint64_t now;

void sched_init(Task *idle) {
    for (int i = 0; i < MAX_TASKS; i++) slots[i] = NULL;
    idle_task = idle;
    idle->state = TASK_RUNNING;
    current = idle;
    current_slot = -1;
    now = 0;
}

int sched_add(Task *t) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (slots[i] == NULL || slots[i]->state == TASK_DEAD) {
            t->state = TASK_READY;
            slots[i] = t;
            return 0;
        }
    }
    return -1;
}

Task *sched_current(void) {
    return current;
}

Task *sched_pick_next(void) {
    if (current->state == TASK_RUNNING) current->state = TASK_READY;
    for (int i = 1; i <= MAX_TASKS; i++) {
        int s = (current_slot + i) % MAX_TASKS;
        if (slots[s] && slots[s]->state == TASK_READY) {
            current = slots[s];
            current_slot = s;
            current->state = TASK_RUNNING;
            return current;
        }
    }
    current = idle_task;
    current_slot = -1;
    current->state = TASK_RUNNING;
    return current;
}

void sched_sleep(uint64_t ticks) {
    if (current == idle_task) return;
    current->state = TASK_SLEEPING;
    current->wake_tick = now + ticks;
}

void sched_tick(void) {
    now++;
    for (int i = 0; i < MAX_TASKS; i++)
        if (slots[i] && slots[i]->state == TASK_SLEEPING && slots[i]->wake_tick <= now) slots[i]->state = TASK_READY;
}

uint64_t sched_now(void) {
    return now;
}

void sched_block(void) {
    if (current != idle_task) current->state = TASK_BLOCKED;
}

void sched_unblock(Task *t) {
    if (t->state == TASK_BLOCKED) t->state = TASK_READY;
}

void sched_exit(void) {
    if (current != idle_task) current->state = TASK_DEAD;
}
