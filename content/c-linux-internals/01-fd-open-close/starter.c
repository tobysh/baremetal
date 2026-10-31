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

// Allocate the lowest free descriptor and a free open file description.
// Return the descriptor, or -1 if either table is full (and change nothing).
int fd_open(sysfiles *s, fdtable *t, int ino) {
    return -1;
}

// Close fd. Return 0, or -1 if fd is out of range or not open.
// The open file description is freed when its last descriptor closes.
int fd_close(sysfiles *s, fdtable *t, int fd) {
    return -1;
}

// Move the shared offset forward by n (n >= 0). Return the new offset, or -1 on a bad fd or n < 0.
long fd_advance(sysfiles *s, fdtable *t, int fd, long n) {
    return -1;
}

int main(void) {
    sysfiles s; fdtable t;
    sys_init(&s); fdt_init(&t);
    int a = fd_open(&s, &t, 7);
    int b = fd_open(&s, &t, 9);
    fd_advance(&s, &t, a, 100);
    fd_close(&s, &t, a);          // fd 0 is free again
    int c = fd_open(&s, &t, 11);  // the lowest free number is reused
    printf("opened %d %d, then %d\n", a, b, c);
    fdt_dump(&s, &t);
    return 0;
}
