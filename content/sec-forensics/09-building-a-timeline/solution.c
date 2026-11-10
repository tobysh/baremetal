#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    long long t;
    char src[64];
    char msg[512];
} Event;

static Event ev[1000];

int main(void) {
    size_t n = 0;
    char line[512];
    while (n < 1000 && fgets(line, sizeof line, stdin)) {
        int at = 0;
        if (sscanf(line, "%lld %63s %n", &ev[n].t, ev[n].src, &at) != 2) continue;
        strcpy(ev[n].msg, line + at);
        ev[n].msg[strcspn(ev[n].msg, "\r\n")] = 0;
        n++;
    }

    /* stable insertion sort: shift only strictly greater timestamps */
    for (size_t i = 1; i < n; i++) {
        Event v = ev[i];
        size_t j = i;
        while (j && ev[j - 1].t > v.t) {
            ev[j] = ev[j - 1];
            j--;
        }
        ev[j] = v;
    }

    for (size_t i = 0; i < n; i++) {
        if (i && ev[i].t - ev[i - 1].t >= 3600) {
            long long gap = ev[i].t - ev[i - 1].t;
            printf("-- quiet for %lldh %02lldm --\n", gap / 3600, gap % 3600 / 60);
        }
        time_t t = (time_t)ev[i].t;
        struct tm tm = *gmtime(&t);
        char stamp[32];
        strftime(stamp, sizeof stamp, "%Y-%m-%dT%H:%M:%SZ", &tm);
        printf("%s [%s] %s\n", stamp, ev[i].src, ev[i].msg);
    }

    long long span = n ? ev[n - 1].t - ev[0].t : 0;
    printf("%zu events over %lldh %02lldm\n", n, span / 3600, span % 3600 / 60);
    return 0;
}
