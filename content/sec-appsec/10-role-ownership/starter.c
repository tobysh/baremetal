#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum { ROLE_GUEST = 0, ROLE_MEMBER = 1, ROLE_EDITOR = 2, ROLE_ADMIN = 3 } Role;
typedef enum { ACT_READ = 0, ACT_EDIT = 1, ACT_DELETE = 2, ACT_SHARE = 3, ACT_CREATE = 4 } Action;
typedef struct { int uid; int org; int role; } User;                  // role is a Role, but may hold garbage
typedef struct { int id; int owner; int org; int is_public; int locked; } Doc;

// BROKEN: any signed-in caller may do anything to anything.
int authorize(const User *u, int action, const Doc *d) {
    (void)action; (void)d;
    return u != NULL;
}

// Fill out_idx with the indexes of the documents u may READ; return how many.
size_t filter_readable(const User *u, const Doc *docs, size_t n, size_t *out_idx) {
    size_t k = 0;
    (void)u;
    for (size_t i = 0; i < n; i++) out_idx[k++] = i;
    return k;
}

int main(void) {
    User u = { 7, 1, ROLE_MEMBER };
    Doc d = { 100, 8, 2, 0, 0 };
    printf("%d\n", authorize(&u, ACT_READ, &d));
    return 0;
}
