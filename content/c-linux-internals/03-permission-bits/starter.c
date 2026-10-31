#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

// 1 if cred c may do `want` (P_R, P_W, P_X or an OR of them) on inode n, else 0.
int may_access(const inode *n, const cred *c, int want) {
    return 1;
}

int main(void) {
    inode secret = {1, 1000, 100, 0640, 0};            // rw-r-----  owner 1000, group 100
    cred owner = {1000, 1000, 100, 100, {0}, 0};
    cred pal   = {2000, 2000, 100, 100, {0}, 0};       // in the file's group
    cred other = {3000, 3000, 300, 300, {0}, 0};
    const char *who[] = {"owner", "group member", "stranger"};
    const cred *cs[] = {&owner, &pal, &other};
    for (int i = 0; i < 3; i++)
        printf("%-12s read %d write %d\n", who[i], may_access(&secret, cs[i], P_R), may_access(&secret, cs[i], P_W));
    return 0;
}
