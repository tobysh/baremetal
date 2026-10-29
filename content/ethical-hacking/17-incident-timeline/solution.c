#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { long ts; int idx; char src[48]; char msg[256]; } Ev;
static Ev ev[1000]; static char sname[50][48]; static long soff[50]; static int ns, ne;
static int phase(const char *msg) {
    char w[64]; size_t n = 0; while (msg[n] && msg[n] != ' ' && n < 63) { w[n] = msg[n]; n++; } w[n] = 0;
    if (!strcmp(w, "scan")) return 0;
    if (!strcmp(w, "login_fail") || !strcmp(w, "login_ok")) return 1;
    if (!strcmp(w, "new_startup") || !strcmp(w, "new_task")) return 2;
    if (!strcmp(w, "hop")) return 3;
    if (!strcmp(w, "big_upload")) return 4;
    return -1;
}
int main(void) {
    char line[512]; int sect = 0;
    while (fgets(line, sizeof line, stdin)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strcmp(line, "# sources")) { sect = 1; continue; }
        if (!strcmp(line, "# events")) { sect = 2; continue; }
        if (sect == 1) { char n[48]; long o; if (sscanf(line, "%47s %ld", n, &o) == 2 && ns < 50) { strcpy(sname[ns], n); soff[ns++] = o; } }
        else if (sect == 2 && ne < 1000) {
            long ts; char src[48]; int used = 0;
            if (sscanf(line, "%ld %47s %n", &ts, src, &used) < 2 || used == 0) continue;
            long off = 0; for (int i = 0; i < ns; i++) if (!strcmp(sname[i], src)) off = soff[i];
            ev[ne].ts = ts - off; ev[ne].idx = ne; strcpy(ev[ne].src, src); snprintf(ev[ne].msg, sizeof ev[ne].msg, "%s", line + used); ne++;
        }
    }
    for (int i = 1; i < ne; i++) { Ev v = ev[i]; int j = i; while (j && ev[j-1].ts > v.ts) { ev[j] = ev[j-1]; j--; } ev[j] = v; }
    puts("TIMELINE");
    for (int i = 0; i < ne; i++) printf("%ld %s %s\n", ev[i].ts, ev[i].src, ev[i].msg);
    static const char *names[5] = { "recon", "access", "persistence", "lateral", "exfil" };
    long first[5], last[5]; int cnt[5] = {0}; long lo = 0, hi = 0; int any = 0;
    for (int i = 0; i < ne; i++) {
        int p = phase(ev[i].msg); if (p < 0) continue;
        if (!cnt[p]) first[p] = ev[i].ts;
        last[p] = ev[i].ts; cnt[p]++;
        if (!any) lo = ev[i].ts; any = 1; hi = ev[i].ts;
    }
    puts("PHASES");
    int ok = 1; long prev = 0; int seen = 0;
    for (int p = 0; p < 5; p++) if (cnt[p]) {
        printf("PHASE %s first=%ld last=%ld events=%d\n", names[p], first[p], last[p], cnt[p]);
        if (seen && first[p] < prev) ok = 0;
        prev = first[p]; seen = 1;
    }
    printf("DWELL %ld\n", any ? hi - lo : 0L);
    printf("SEQUENCE %s\n", ok ? "in-order" : "out-of-order");
    return 0;
}
