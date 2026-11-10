#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    char ip[64];
    int hits;
} Source;

static Source src[500];

static int by_hits(const void *a, const void *b) {
    const Source *x = *(Source *const *)a, *y = *(Source *const *)b;
    if (x->hits != y->hits) return y->hits - x->hits;
    return strcmp(x->ip, y->ip);
}

int main(void) {
    char line[2048];
    int class_count[6] = {0}, malformed = 0, n = 0;

    while (fgets(line, sizeof line, stdin)) {
        char ip[64];
        if (sscanf(line, "%63s", ip) != 1) { malformed++; continue; }

        char *q1 = strchr(line, '"');
        char *q2 = q1 ? strchr(q1 + 1, '"') : NULL;
        if (!q2) { malformed++; continue; }
        *q2 = 0;

        char method[16], path[512], version[16], extra[2];
        if (sscanf(q1 + 1, "%15s %511s %15s %1s", method, path, version, extra) != 3) { malformed++; continue; }

        int status;
        char size[32], extra2[2];
        if (sscanf(q2 + 1, " %d %31s %1s", &status, size, extra2) != 2 || status < 100 || status > 599) { malformed++; continue; }

        int size_ok = strcmp(size, "-") == 0;
        if (!size_ok) {
            size_ok = size[0] != 0;
            for (char *p = size; *p; p++)
                if (*p < '0' || *p > '9') size_ok = 0;
        }
        if (!size_ok) { malformed++; continue; }

        class_count[status / 100]++;

        for (char *p = path; *p; p++)
            if (*p >= 'A' && *p <= 'Z') *p += 32;
        if (strstr(path, "../") || strstr(path, "%2e%2e") || strstr(path, "/etc/passwd")) {
            int j = 0;
            while (j < n && strcmp(src[j].ip, ip)) j++;
            if (j == n) {
                if (n == 500) continue;
                strcpy(src[n++].ip, ip);
            }
            src[j].hits++;
        }
    }

    printf("2xx=%d 3xx=%d 4xx=%d 5xx=%d malformed=%d\n", class_count[2], class_count[3], class_count[4], class_count[5], malformed);

    Source *idx[500];
    int c = 0;
    for (int i = 0; i < n; i++)
        if (src[i].hits >= 2) idx[c++] = &src[i];
    qsort(idx, c, sizeof idx[0], by_hits);
    for (int i = 0; i < c; i++) printf("SUSPECT %s hits=%d\n", idx[i]->ip, idx[i]->hits);
    return 0;
}
