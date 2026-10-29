#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { long ts; char dst[48], user[48], src[48]; } Ev;
static Ev ev[1000]; static int parent[1000], isparent[1000];
int main(void) {
    char line[400]; int n = 0, printed = 0;
    while (fgets(line, sizeof line, stdin) && n < 1000) {
        Ev e; char res[16];
        if (sscanf(line, "%ld %47s %47s %47s %15s", &e.ts, e.dst, e.user, e.src, res) != 5) continue;
        if (strcmp(res, "OK")) continue;
        ev[n] = e; parent[n] = -1;
        for (int p = n - 1; p >= 0; p--) if (!strcmp(ev[p].dst, e.src) && e.ts - ev[p].ts <= 600) { parent[n] = p; isparent[p] = 1; break; }
        n++;
    }
    for (int i = 0; i < n; i++) {
        if (isparent[i] || parent[i] < 0) continue;
        int chain[1000], k = 0;
        for (int c = i; c >= 0; c = parent[c]) chain[k++] = c;
        printf("CHAIN %s", ev[chain[k - 1]].src);
        for (int j = k - 1; j >= 0; j--) printf(" > %s", ev[chain[j]].dst);
        printf(" users=");
        char seen[1000][48]; int ns = 0;
        for (int j = k - 1; j >= 0; j--) {
            int f = 0; for (int s = 0; s < ns; s++) if (!strcmp(seen[s], ev[chain[j]].user)) f = 1;
            if (!f) { printf("%s%s", ns ? "," : "", ev[chain[j]].user); strcpy(seen[ns++], ev[chain[j]].user); }
        }
        printf(" start=%ld end=%ld hops=%d\n", ev[chain[k - 1]].ts, ev[i].ts, k);
        printed++;
    }
    if (!printed) puts("no lateral movement found");
    return 0;
}
