#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { long ts; char dst[48], user[48], src[48]; } Ev;
typedef struct { char src[48]; int fails; char hit[160]; } Src;
static Ev ok[1000]; static Src s[1000];
int main(void) {
    char line[400]; int nok = 0, ns = 0, pivots = 0, rows = 0;
    while (fgets(line, sizeof line, stdin)) {
        Ev e; char res[16];
        if (sscanf(line, "%ld %47s %47s %47s %15s", &e.ts, e.dst, e.user, e.src, res) != 5) continue;
        int k = 0; while (k < ns && strcmp(s[k].src, e.src)) k++;
        if (k == ns) { if (ns == 1000) continue; strcpy(s[ns++].src, e.src); }
        if (!strcmp(res, "FAIL")) { s[k].fails++; continue; }
        if (strcmp(res, "OK")) continue;
        if (s[k].fails >= 5 && !s[k].hit[0]) snprintf(s[k].hit, sizeof s[k].hit, " compromised user=%s host=%s at=%ld", e.user, e.dst, e.ts);
        for (int p = nok - 1; p >= 0; p--) if (!strcmp(ok[p].dst, e.src) && e.ts - ok[p].ts <= 600) { pivots++; break; }
        if (nok < 1000) ok[nok++] = e;
    }
    int order[1000], m = 0;
    for (int i = 0; i < ns; i++) if (s[i].fails >= 5) order[m++] = i;
    for (int i = 1; i < m; i++) { int v = order[i], j = i; while (j && (s[order[j-1]].fails < s[v].fails || (s[order[j-1]].fails == s[v].fails && strcmp(s[order[j-1]].src, s[v].src) > 0))) { order[j] = order[j-1]; j--; } order[j] = v; }
    if (!m) puts("BRUTE none");
    for (int i = 0; i < m; i++) { Src *x = &s[order[i]]; printf("BRUTE %s fails=%d%s\n", x->src, x->fails, x->hit); rows++; }
    printf("PIVOTS %d\n", pivots);
    return 0;
}
