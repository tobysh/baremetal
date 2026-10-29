#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { long ts; char src[48]; int allow; int port; } Ev;
static Ev ev[2000];
typedef struct { char src[48]; long at; int ports, allowed; } Rep;
int main(void) {
    char line[256]; int n = 0, nr = 0;
    static char seen[64][48]; int ns = 0; Rep rep[64];
    while (fgets(line, sizeof line, stdin) && n < 2000) {
        long ts; char src[48], act[16], proto[16]; int port;
        if (sscanf(line, "%ld %47s %15s %15s %d", &ts, src, act, proto, &port) != 5) continue;
        ev[n].ts = ts; strcpy(ev[n].src, src); ev[n].allow = !strcmp(act, "ALLOW"); ev[n].port = port; n++;
        int s = 0; while (s < ns && strcmp(seen[s], src)) s++;
        if (s == ns) { if (ns == 64) continue; strcpy(seen[ns++], src); }
        int ports[2000], allowed[2000], np = 0;
        for (int i = 0; i < n; i++) {
            if (strcmp(ev[i].src, src) || ev[i].ts <= ts - 10) continue;
            int k = 0; while (k < np && ports[k] != ev[i].port) k++;
            if (k == np) { ports[np] = ev[i].port; allowed[np++] = 0; }
            if (ev[i].allow) allowed[k] = 1;
        }
        int dup = 0; for (int r = 0; r < nr; r++) if (!strcmp(rep[r].src, src)) dup = 1;
        if (np >= 8 && !dup) {
            strcpy(rep[nr].src, src); rep[nr].at = ts; rep[nr].ports = np;
            int a = 0; for (int k = 0; k < np; k++) a += allowed[k];
            rep[nr].allowed = a; nr++;
        }
    }
    if (!nr) { puts("no scan detected"); return 0; }
    for (int i = 1; i < nr; i++) { Rep v = rep[i]; int j = i; while (j && (rep[j-1].at > v.at || (rep[j-1].at == v.at && strcmp(rep[j-1].src, v.src) > 0))) { rep[j] = rep[j-1]; j--; } rep[j] = v; }
    for (int i = 0; i < nr; i++) printf("SCAN %s at=%ld ports=%d allowed=%d\n", rep[i].src, rep[i].at, rep[i].ports, rep[i].allowed);
    return 0;
}
