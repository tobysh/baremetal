#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    int (*exec_text)(void *db, const char *sql, int *ids, int cap);
    int (*exec_param)(void *db, const char *sql, const char *param, int *ids, int cap);
} DbApi;

// VULNERABLE: the name is pasted into the query text.
int find_user_ids(const DbApi *api, void *db, const char *name, int *ids, int cap) {
    char sql[256];
    if (!name) return 0;
    snprintf(sql, sizeof sql, "SELECT id FROM users WHERE name='%s'", name);
    return api->exec_text(db, sql, ids, cap);
}

int looks_like_injection(const char *input) {
    (void)input;
    return 0;
}

int main(void) {
    puts("run the tests to see the toy database misbehave");
    return 0;
}
