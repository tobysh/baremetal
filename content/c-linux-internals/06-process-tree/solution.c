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

int pt_fork(ptable *t, int parent) {
    if (parent < 1 || parent >= NPROC || t->p[parent].state != P_RUN) return -1;
    for (int pid = 2; pid < NPROC; pid++) {
        if (t->p[pid].state == P_FREE) { t->p[pid] = (proc){P_RUN, parent, 0}; return pid; }
    }
    return -1;
}

int pt_exit(ptable *t, int pid, int status) {
    if (pid <= 1 || pid >= NPROC || t->p[pid].state != P_RUN) return -1;
    for (int q = 2; q < NPROC; q++) {
        if (q != pid && t->p[q].state != P_FREE && t->p[q].ppid == pid) t->p[q].ppid = 1;
    }
    t->p[pid].state = P_ZOMBIE;
    t->p[pid].status = status;
    return 0;
}

int pt_wait(ptable *t, int parent, int *status) {
    if (parent < 1 || parent >= NPROC || t->p[parent].state != P_RUN) return -1;
    int kids = 0;
    for (int pid = 2; pid < NPROC; pid++) {
        if (t->p[pid].state == P_FREE || t->p[pid].ppid != parent) continue;
        kids++;
        if (t->p[pid].state == P_ZOMBIE) {
            if (status) *status = t->p[pid].status;
            t->p[pid] = (proc){P_FREE, 0, 0};
            return pid;
        }
    }
    return kids ? 0 : -1;
}

int pt_descendants(const ptable *t, int pid) {
    if (pid < 1 || pid >= NPROC || t->p[pid].state == P_FREE) return -1;
    int n = 0;
    for (int q = 1; q < NPROC; q++) {
        if (q == pid || t->p[q].state == P_FREE) continue;
        int a = t->p[q].ppid;
        for (int hops = 0; a > 0 && a < NPROC && hops < NPROC; hops++) {
            if (a == pid) { n++; break; }
            a = t->p[a].ppid;
        }
    }
    return n;
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
