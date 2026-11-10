#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

static uint64_t fnv(const char *s) {
    uint64_t h = 14695981039346656037ULL;
    for (; *s; s++) {
        h ^= (uint8_t)*s;
        h *= 1099511628211ULL;
    }
    return h;
}

/* Split line in place on '|'. Returns the number of fields (5 means "more than four"). */
static int split4(char *line, char *f[4]) {
    int nf = 0;
    f[nf++] = line;
    for (char *p = line; *p; p++) {
        if (*p != '|') continue;
        *p = 0;
        if (nf == 4) return 5;
        f[nf++] = p + 1;
    }
    return nf;
}

static int well_formed(char *f[4]) {
    size_t sl = strlen(f[0]);
    if (sl == 0 || sl > 9 || f[0][0] == '0') return 0;
    for (size_t i = 0; i < sl; i++)
        if (f[0][i] < '0' || f[0][i] > '9') return 0;
    if (!f[1][0] || !f[2][0]) return 0;
    if (strlen(f[3]) != 16) return 0;
    for (int i = 0; i < 16; i++) {
        char c = f[3][i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
    }
    return 1;
}

int main(void) {
    char line[1024];
    char prev[17] = "0000000000000000";
    unsigned long expected = 1;
    int lineno = 0;

    while (fgets(line, sizeof line, stdin)) {
        lineno++;
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;

        char *f[4];
        if (split4(line, f) != 4 || !well_formed(f)) {
            printf("BROKEN at line %d: malformed\n", lineno);
            return 0;
        }
        unsigned long seq = strtoul(f[0], NULL, 10);
        if (seq != expected) {
            printf("BROKEN at line %d: sequence (expected %lu, found %lu)\n", lineno, expected, seq);
            return 0;
        }

        char text[1024], want[17];
        snprintf(text, sizeof text, "%s|%lu|%s|%s", prev, seq, f[1], f[2]);
        snprintf(want, sizeof want, "%016llx", (unsigned long long)fnv(text));
        if (strcmp(want, f[3]) != 0) {
            printf("BROKEN at line %d: hash mismatch\n", lineno);
            return 0;
        }
        strcpy(prev, f[3]);
        expected++;
    }
    printf("chain ok: %lu entries head=%s\n", expected - 1, prev);
    return 0;
}
