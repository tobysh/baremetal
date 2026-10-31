#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NFD  8     // descriptors per process
#define NOFD 8     // open file descriptions in the whole "kernel"

// An open file description: what a descriptor number points at.
// refs == 0 means the slot is free.
typedef struct { int ino; long off; int refs; } ofd;

// System-wide table of open file descriptions.
typedef struct { ofd files[NOFD]; } sysfiles;

// Per-process descriptor table. slot[fd] is an index into sysfiles.files, or -1 when closed.
typedef struct { int slot[NFD]; } fdtable;

void sys_init(sysfiles *s) { memset(s, 0, sizeof *s); }

void fdt_init(fdtable *t) {
    for (int i = 0; i < NFD; i++) t->slot[i] = -1;
}

static void fdt_dump(const sysfiles *s, const fdtable *t) {
    for (int fd = 0; fd < NFD; fd++) {
        if (t->slot[fd] < 0) continue;
        const ofd *f = &s->files[t->slot[fd]];
        printf("fd %d -> file %d (ino %d, offset %ld, refs %d)\n", fd, t->slot[fd], f->ino, f->off, f->refs);
    }
}

static int fd_valid(const fdtable *t, int fd) {
    return fd >= 0 && fd < NFD && t->slot[fd] >= 0;
}

static void file_drop(sysfiles *s, int f) {
    if (--s->files[f].refs == 0) s->files[f] = (ofd){0, 0, 0};
}

int fd_open(sysfiles *s, fdtable *t, int ino) {
    int fd = -1, f = -1;
    for (int i = 0; i < NFD && fd < 0; i++) if (t->slot[i] < 0) fd = i;
    for (int j = 0; j < NOFD && f < 0; j++) if (s->files[j].refs == 0) f = j;
    if (fd < 0 || f < 0) return -1;
    s->files[f] = (ofd){ino, 0, 1};
    t->slot[fd] = f;
    return fd;
}

int fd_close(sysfiles *s, fdtable *t, int fd) {
    if (!fd_valid(t, fd)) return -1;
    file_drop(s, t->slot[fd]);
    t->slot[fd] = -1;
    return 0;
}

long fd_advance(sysfiles *s, fdtable *t, int fd, long n) {
    if (!fd_valid(t, fd) || n < 0) return -1;
    s->files[t->slot[fd]].off += n;
    return s->files[t->slot[fd]].off;
}

int fd_dup(sysfiles *s, fdtable *t, int fd) {
    if (!fd_valid(t, fd)) return -1;
    for (int i = 0; i < NFD; i++) {
        if (t->slot[i] < 0) {
            t->slot[i] = t->slot[fd];
            s->files[t->slot[fd]].refs++;
            return i;
        }
    }
    return -1;
}

int fd_dup2(sysfiles *s, fdtable *t, int oldfd, int newfd) {
    if (!fd_valid(t, oldfd) || newfd < 0 || newfd >= NFD) return -1;
    if (oldfd == newfd) return newfd;
    if (t->slot[newfd] >= 0) fd_close(s, t, newfd);
    t->slot[newfd] = t->slot[oldfd];
    s->files[t->slot[oldfd]].refs++;
    return newfd;
}


#define P_R 4
#define P_W 2
#define P_X 1
#define MODE_STICKY 01000
#define MODE_SETGID 02000
#define MODE_SETUID 04000

typedef struct { int ino; unsigned uid, gid; unsigned mode; int is_dir; } inode;
typedef struct { unsigned uid, euid, gid, egid; unsigned groups[4]; int ngroups; } cred;

static int in_group(const cred *c, unsigned gid) {
    if (c->egid == gid) return 1;
    for (int i = 0; i < c->ngroups; i++) if (c->groups[i] == gid) return 1;
    return 0;
}

int may_access(const inode *n, const cred *c, int want) {
    if (c->euid == 0) {
        // root skips the mode bits, but execute needs some x bit on a plain file
        if ((want & P_X) && !n->is_dir && !(n->mode & 0111)) return 0;
        return 1;
    }
    unsigned bits;
    if (c->euid == n->uid) bits = (n->mode >> 6) & 7;      // only the owner class applies
    else if (in_group(c, n->gid)) bits = (n->mode >> 3) & 7;
    else bits = n->mode & 7;
    return (bits & want) == want;
}


#define NSIG 32            // signal numbers 1..31
#define SG_HUP   1
#define SG_INT   2
#define SG_KILL  9
#define SG_USR1  10
#define SG_USR2  12
#define SG_TERM  15
#define SG_CHLD  17
#define SG_CONT  18
#define SG_STOP  19
#define SG_TSTP  20

enum { D_DFL, D_IGN, D_CATCH };            // disposition
enum { A_TERM, A_IGN, A_STOP };            // default action
enum { M_BLOCK, M_UNBLOCK, M_SETMASK };    // how to change the mask

typedef struct {
    int disp[NSIG];
    uint32_t pending, blocked;             // bit n is signal n
    int dead, term_sig, stopped;
    int handled[32]; int nhandled;         // log of signals whose handler "ran"
} sproc;

#define UNBLOCKABLE ((1u << SG_KILL) | (1u << SG_STOP))

static uint32_t bit(int sig) { return 1u << sig; }

// Default actions: HUP INT KILL USR1 USR2 TERM etc. terminate; CHLD and CONT are ignored;
// STOP and TSTP stop. Any signal number not listed terminates.
static int default_action(int sig) {
    if (sig == SG_CHLD || sig == SG_CONT) return A_IGN;
    if (sig == SG_STOP || sig == SG_TSTP) return A_STOP;
    return A_TERM;
}

static int is_ignored(const sproc *p, int sig) {
    return p->disp[sig] == D_IGN || (p->disp[sig] == D_DFL && default_action(sig) == A_IGN);
}

void sig_init(sproc *p) { memset(p, 0, sizeof *p); }

int sig_set_disp(sproc *p, int sig, int disp) {
    if (sig < 1 || sig >= NSIG || sig == SG_KILL || sig == SG_STOP) return -1;
    if (disp != D_DFL && disp != D_IGN && disp != D_CATCH) return -1;
    p->disp[sig] = disp;
    if (is_ignored(p, sig)) p->pending &= ~bit(sig);
    return 0;
}

uint32_t sig_mask(sproc *p, int how, uint32_t set) {
    uint32_t old = p->blocked;
    set &= ~(UNBLOCKABLE | 1u);
    if (how == M_BLOCK) p->blocked |= set;
    else if (how == M_UNBLOCK) p->blocked &= ~set;
    else if (how == M_SETMASK) p->blocked = set;
    return old;
}

int sig_send(sproc *p, int sig) {
    if (sig < 1 || sig >= NSIG) return -1;
    if (p->dead) return 0;
    if (sig == SG_CONT) { p->stopped = 0; p->pending &= ~(bit(SG_STOP) | bit(SG_TSTP)); }
    if (sig == SG_STOP || sig == SG_TSTP) p->pending &= ~bit(SG_CONT);
    if (is_ignored(p, sig) && !(p->blocked & bit(sig))) return 0;   // discarded at once
    p->pending |= bit(sig);                                          // standard signals coalesce
    return 0;
}

int sig_deliver(sproc *p) {
    if (p->dead) return 0;
    uint32_t ready = p->pending & ~p->blocked;
    if (p->stopped) ready &= bit(SG_KILL);          // a stopped process can only be killed
    if (!ready) return 0;
    int sig = 1;
    while (!(ready & bit(sig))) sig++;
    p->pending &= ~bit(sig);
    if (sig == SG_KILL) { p->dead = 1; p->term_sig = sig; p->pending = 0; return sig; }
    if (p->disp[sig] == D_CATCH) {
        if (p->nhandled < 32) p->handled[p->nhandled++] = sig;
    } else if (p->disp[sig] == D_DFL) {
        int a = default_action(sig);
        if (a == A_TERM) { p->dead = 1; p->term_sig = sig; p->pending = 0; }
        else if (a == A_STOP) p->stopped = 1;
    }
    return sig;
}

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

// ---- the simulated kernel state ----
typedef struct {
    sysfiles files;               // open file descriptions (lesson 1)
    fdtable  fdt[NPROC];          // one descriptor table per pid
    ptable   pt;                  // process table (lesson 6)
    sproc    sig[NPROC];          // signal state per pid (lesson 5)
    cred     cr[NPROC];           // credentials per pid (lessons 3-4)
} kstate;

void ks_init(kstate *k) {
    memset(k, 0, sizeof *k);
    sys_init(&k->files);
    pt_init(&k->pt);
    for (int i = 0; i < NPROC; i++) { fdt_init(&k->fdt[i]); sig_init(&k->sig[i]); }
}

// Violation bits returned by ks_check.
#define V_DANGLING        (1u << 0)  // a running process's descriptor points outside the table or at a free description
#define V_REFCOUNT        (1u << 1)  // a description's refs is negative, or positive and not equal to the descriptors naming it
#define V_DEAD_FDS        (1u << 2)  // a zombie or free process still has an open descriptor
#define V_PARENT          (1u << 3)  // a pid >= 2 that exists has a parent that is not a running process
#define V_CYCLE           (1u << 4)  // following parents from some process never reaches init
#define V_SIG_DEAD        (1u << 5)  // signals are pending for a zombie or free process
#define V_SIG_UNBLOCKABLE (1u << 6)  // a live process blocks SIGKILL, SIGSTOP or bit 0
#define V_INIT            (1u << 7)  // pid 1 is not running, or has a parent

int ks_open(kstate *k, int pid, const inode *n, int want) {
    if (pid < 1 || pid >= NPROC || k->pt.p[pid].state != P_RUN) return -1;
    if (!may_access(n, &k->cr[pid], want)) return -1;
    return fd_open(&k->files, &k->fdt[pid], n->ino);
}

int ks_exit(kstate *k, int pid, int status) {
    if (pt_exit(&k->pt, pid, status) != 0) return -1;
    for (int fd = 0; fd < NFD; fd++) fd_close(&k->files, &k->fdt[pid], fd);
    k->sig[pid].pending = 0;
    sig_send(&k->sig[k->pt.p[pid].ppid], SG_CHLD);
    return 0;
}

uint32_t ks_check(const kstate *k) {
    uint32_t v = 0;
    int count[NOFD] = {0};
    for (int pid = 1; pid < NPROC; pid++) {
        const proc *p = &k->pt.p[pid];
        for (int fd = 0; fd < NFD; fd++) {
            int s = k->fdt[pid].slot[fd];
            if (s == -1) continue;
            int valid = s >= 0 && s < NOFD;
            if (valid) count[s]++;
            if (p->state == P_RUN && (!valid || k->files.files[s].refs <= 0)) v |= V_DANGLING;
            if (p->state != P_RUN) v |= V_DEAD_FDS;
        }
        if (p->state == P_FREE) {
            if (k->sig[pid].pending) v |= V_SIG_DEAD;
            continue;
        }
        if (p->state == P_ZOMBIE && k->sig[pid].pending) v |= V_SIG_DEAD;
        if (k->sig[pid].blocked & (UNBLOCKABLE | 1u)) v |= V_SIG_UNBLOCKABLE;
        if (pid >= 2) {
            if (p->ppid < 1 || p->ppid >= NPROC || k->pt.p[p->ppid].state != P_RUN) v |= V_PARENT;
            int cur = pid, steps = 0;
            while (cur != 1) {
                if (cur < 1 || cur >= NPROC || k->pt.p[cur].state == P_FREE) break;
                cur = k->pt.p[cur].ppid;
                if (++steps > NPROC) { v |= V_CYCLE; break; }
            }
        }
    }
    for (int j = 0; j < NOFD; j++) {
        int r = k->files.files[j].refs;
        if (r < 0 || (r > 0 && r != count[j])) v |= V_REFCOUNT;
    }
    if (k->pt.p[1].state != P_RUN || k->pt.p[1].ppid != 0) v |= V_INIT;
    return v;
}

int main(void) {
    kstate k; ks_init(&k);
    inode log = {50, 0, 0, 0644, 0};
    int shell = pt_fork(&k.pt, 1);
    k.cr[shell] = (cred){1000, 1000, 100, 100, {0}, 0};
    int fd = ks_open(&k, shell, &log, P_R);
    int job = pt_fork(&k.pt, shell);
    printf("shell %d opened fd %d (read allowed for others: mode 0644)\n", shell, fd);
    printf("consistent state: violations %#x\n", ks_check(&k));
    ks_exit(&k, job, 0);
    printf("after the job exits: violations %#x, job is a zombie of pid %d\n", ks_check(&k), k.pt.p[job].ppid);
    k.files.files[k.fdt[shell].slot[fd]].refs = 5;          // corrupt a reference count
    printf("after corrupting a refcount: violations %#x (expect %#x)\n", ks_check(&k), V_REFCOUNT);
    return 0;
}
