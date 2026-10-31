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

// 1 if c may remove (unlink) `file` from directory `dir`.
int may_unlink(const inode *dir, const inode *file, const cred *c) {
    return 1;
}

// Run program `prog` as c: returns -1 (and changes nothing) if c may not execute it
// or it is a directory. Otherwise applies setuid/setgid and returns 0.
int do_exec(const inode *prog, cred *c) {
    return 0;
}

int main(void) {
    inode tmp   = {1, 0, 0, 01777, 1};            // like /tmp: everyone may write, sticky bit set
    inode alice = {2, 1000, 100, 0644, 0};        // alice's file in it
    cred alice_c = {1000, 1000, 100, 100, {0}, 0};
    cred bob_c   = {2000, 2000, 200, 200, {0}, 0};
    printf("alice removes her file: %d\n", may_unlink(&tmp, &alice, &alice_c));
    printf("bob removes alice's file: %d\n", may_unlink(&tmp, &alice, &bob_c));

    inode helper = {3, 0, 0, 04755, 0};           // a fictional setuid-root helper
    cred bob2 = bob_c;
    int r = do_exec(&helper, &bob2);
    printf("exec -> %d, uid %u, euid %u\n", r, bob2.uid, bob2.euid);
    return 0;
}
