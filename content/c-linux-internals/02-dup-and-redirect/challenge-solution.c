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

int fd_dup3(sysfiles *s, fdtable *t, int oldfd, int newfd, int flag) {
    if (oldfd == newfd) return -1;
    int r = fd_dup2(s, t, oldfd, newfd);
    if (r < 0) return -1;
    return r | (flag ? 0x100 : 0);
}

int fd_closefrom(sysfiles *s, fdtable *t, int lo) {
    int n = 0;
    for (int i = lo < 0 ? 0 : lo; i < NFD; i++) if (fd_close(s, t, i) == 0) n++;
    return n;
}

int main(void) { return 0; }
