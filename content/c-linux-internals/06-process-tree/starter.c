#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NPROC 16                       // pids 1..15; pid 1 is init, slot 0 is unused
enum { P_FREE, P_RUN, P_ZOMBIE };

typedef struct { int state, ppid, status; } proc;
typedef struct { proc p[NPROC]; } ptable;

void pt_init(ptable *t) {
    memset(t, 0, sizeof *t);
    t->p[1] = (proc){P_RUN, 0, 0};     // init: running, no parent
}

// New child of `parent` in the lowest free pid >= 2. Returns the pid, or -1 if the parent
// is not a running process or the table is full.
int pt_fork(ptable *t, int parent) {
    return -1;
}

// Process `pid` exits with `status`. It becomes a zombie; its children (running or zombie)
// are adopted by init (pid 1). Returns 0, or -1 if pid is 1, invalid, or not running.
int pt_exit(ptable *t, int pid, int status) {
    return -1;
}

// Reap one zombie child of `parent` (the lowest pid): free its slot, store its status in
// *status (if not NULL) and return its pid. Returns 0 if the parent has children but none
// is a zombie, and -1 if it has no children (or is not running).
int pt_wait(ptable *t, int parent, int *status) {
    return -1;
}

// Number of living or zombie processes below `pid` (children, grandchildren, ...).
// -1 if pid is not an existing process.
int pt_descendants(const ptable *t, int pid) {
    return -1;
}

static void show(const ptable *t) {
    for (int i = 1; i < NPROC; i++)
        if (t->p[i].state != P_FREE) printf("  pid %d parent %d %s\n", i, t->p[i].ppid, t->p[i].state == P_ZOMBIE ? "zombie" : "running");
}

int main(void) {
    ptable t; pt_init(&t);
    int shell = pt_fork(&t, 1), job = pt_fork(&t, shell), helper = pt_fork(&t, job);
    printf("shell %d, job %d, helper %d; descendants of the shell: %d\n", shell, job, helper, pt_descendants(&t, shell));
    pt_exit(&t, job, 3);                       // the job exits; its child is orphaned
    show(&t);
    int st = -1, r = pt_wait(&t, shell, &st);
    printf("shell reaped pid %d with status %d\n", r, st);
    show(&t);
    return 0;
}
