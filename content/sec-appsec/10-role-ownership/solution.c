#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum { ROLE_GUEST = 0, ROLE_MEMBER = 1, ROLE_EDITOR = 2, ROLE_ADMIN = 3 } Role;
typedef enum { ACT_READ = 0, ACT_EDIT = 1, ACT_DELETE = 2, ACT_SHARE = 3, ACT_CREATE = 4 } Action;
typedef struct { int uid; int org; int role; } User;                  // role is a Role, but may hold garbage
typedef struct { int id; int owner; int org; int is_public; int locked; } Doc;

int authorize(const User *u, int action, const Doc *d) {
    if (!u) return 0;
    if (u->role < ROLE_GUEST || u->role > ROLE_ADMIN) return 0;
    if (action < ACT_READ || action > ACT_CREATE) return 0;
    if (action == ACT_CREATE) return d == NULL && u->role >= ROLE_MEMBER;
    if (!d) return 0;
    if (action == ACT_READ && d->is_public) return 1;
    if (u->org != d->org) return 0;
    if (d->locked && action != ACT_READ) return 0;
    if (u->role == ROLE_GUEST) return 0;
    int owner = u->uid == d->owner;
    switch (action) {
    case ACT_READ: return 1;
    case ACT_EDIT: return owner || u->role >= ROLE_EDITOR;
    case ACT_DELETE:
    case ACT_SHARE: return owner || u->role == ROLE_ADMIN;
    }
    return 0;
}

size_t filter_readable(const User *u, const Doc *docs, size_t n, size_t *out_idx) {
    size_t k = 0;
    for (size_t i = 0; i < n; i++) if (authorize(u, ACT_READ, &docs[i])) out_idx[k++] = i;
    return k;
}

int main(void) {
    User u = { 7, 1, ROLE_MEMBER };
    Doc d = { 100, 8, 2, 0, 0 };
    printf("%d\n", authorize(&u, ACT_READ, &d));
    return 0;
}
