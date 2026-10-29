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

int find_user_ids(const DbApi *api, void *db, const char *name, int *ids, int cap) {
    if (!name) return 0;
    return api->exec_param(db, "SELECT id FROM users WHERE name = ?", name, ids, cap);
}

static int has(const char *s, const char *pat) {
    size_t n = strlen(pat);
    for (; *s; s++) {
        size_t i = 0;
        while (i < n && s[i] && tolower((unsigned char)s[i]) == pat[i]) i++;
        if (i == n) return 1;
    }
    return 0;
}

int looks_like_injection(const char *input) {
    if (!input) return 0;
    return strchr(input, '\'') || strstr(input, "--") || strchr(input, ';') || strstr(input, "/*") || has(input, " or ") || has(input, " union ");
}

int main(void) {
    puts("run the tests to see the toy database misbehave");
    return 0;
}
