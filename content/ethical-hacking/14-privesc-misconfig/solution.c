#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { unsigned mode; char owner[32], group[32], path[128]; } Ent;
static Ent ents[1000]; static int ne;
typedef struct { char kind; char label[40], user[32], path[128]; } Job;
static Job jobs[1000]; static int nj;
static char groups[32][32]; static int ng;
static Ent *lookup(const char *p) { for (int i = 0; i < ne; i++) if (!strcmp(ents[i].path, p)) return &ents[i]; return NULL; }
static int ingroup(const char *g) { for (int i = 0; i < ng; i++) if (!strcmp(groups[i], g)) return 1; return 0; }
static int gw(const Ent *e) { return (!strcmp(e->owner, "guest") && (e->mode & 0200)) || (ingroup(e->group) && (e->mode & 020)) || (e->mode & 02); }
static void fix(const Ent *e, char *out) {
    if (!strcmp(e->owner, "guest") && (e->mode & 0200)) { strcpy(out, "chown root"); return; }
    unsigned m = e->mode & ~02u;
    if (ingroup(e->group)) m &= ~020u;
    sprintf(out, "chmod %04o", m);
}
int main(void) {
    char line[512]; int printed = 0;
    while (fgets(line, sizeof line, stdin)) {
        char k[8], a[128], b[128], c[128], d[128];
        int n = sscanf(line, "%7s %127s %127s %127s %127s", k, a, b, c, d);
        if (n >= 3 && !strcmp(k, "G") && !strcmp(a, "guest") && ng < 32) strcpy(groups[ng++], b);
        else if (n == 5 && (k[0] == 'F' || k[0] == 'D') && !k[1] && ne < 1000) {
            char *end; unsigned long m = strtoul(a, &end, 8); if (*end) continue;
            ents[ne].mode = (unsigned)m; strcpy(ents[ne].owner, b); strcpy(ents[ne].group, c); strcpy(ents[ne].path, d); ne++;
        } else if (n == 4 && !strcmp(k, "S") && nj < 1000) { jobs[nj].kind = 'S'; strcpy(jobs[nj].label, a); strcpy(jobs[nj].user, b); strcpy(jobs[nj].path, c); nj++; }
        else if (n == 3 && !strcmp(k, "C") && nj < 1000) { jobs[nj].kind = 'C'; strcpy(jobs[nj].label, "cron"); strcpy(jobs[nj].user, a); strcpy(jobs[nj].path, b); nj++; }
    }
    for (int i = 0; i < nj; i++) {
        if (!strcmp(jobs[i].user, "guest")) continue;
        Ent *e = lookup(jobs[i].path); if (!e) continue;
        char f[64];
        if (gw(e)) { fix(e, f); printf("writable-exec %s path=%s fix=%s\n", jobs[i].label, e->path, f); printed++; continue; }
        char dir[128]; strcpy(dir, e->path); char *s = strrchr(dir, '/'); if (!s) continue;
        if (s == dir) dir[1] = 0; else *s = 0;
        Ent *d = lookup(dir);
        if (d && gw(d) && !(d->mode & 01000)) { fix(d, f); printf("writable-dir %s dir=%s fix=%s\n", jobs[i].label, d->path, f); printed++; }
    }
    for (int i = 0; i < ne; i++) if ((ents[i].mode & 04000) && !strcmp(ents[i].owner, "root") && gw(&ents[i])) {
        char f[64]; fix(&ents[i], f); printf("setuid-writable path=%s fix=%s\n", ents[i].path, f); printed++;
    }
    if (!printed) puts("no privilege escalation paths found");
    return 0;
}
