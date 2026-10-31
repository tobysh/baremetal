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
