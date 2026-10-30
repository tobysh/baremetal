#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { char kind[16], name[64], path[128], user[32]; } E;
static E base[500], cur[500];
static int nb, nc;
static int find(E *a, int n, const E *e) { for (int i = 0; i < n; i++) if (!strcmp(a[i].kind, e->kind) && !strcmp(a[i].name, e->name)) return i; return -1; }
static int score(const E *e, char *flags) {
    int pts = 0; flags[0] = 0;
    const char *sep = "";
    if (!strncmp(e->path, "/tmp/", 5) || !strncmp(e->path, "/var/tmp/", 9) || !strncmp(e->path, "/dev/shm/", 9)) { strcat(flags, "temp-path"); sep = ","; pts += 3; }
    else if (!strncmp(e->path, "/home/", 6)) { strcat(flags, "home-path"); sep = ","; pts += 2; }
    const char *s = strrchr(e->path, '/'); s = s ? s + 1 : e->path;
    if (*s == '.') { strcat(flags, sep); strcat(flags, "hidden"); sep = ","; pts += 2; }
    if (!strcmp(e->user, "root")) { strcat(flags, sep); strcat(flags, "root"); sep = ","; pts += 1; }
    if (!flags[0]) strcpy(flags, "-");
    return pts;
}
int main(void) {
    char line[512]; int sect = 0, printed = 0;
    while (fgets(line, sizeof line, stdin)) {
        if (!strncmp(line, "# baseline", 10)) { sect = 1; continue; }
        if (!strncmp(line, "# current", 9)) { sect = 2; continue; }
        E e;
        if (sscanf(line, "%15s %63s %127s %31s", e.kind, e.name, e.path, e.user) != 4) continue;
        if (sect == 1 && nb < 500) base[nb++] = e;
        else if (sect == 2 && nc < 500) cur[nc++] = e;
    }
    for (int i = 0; i < nc; i++) {
        int j = find(base, nb, &cur[i]); char flags[64];
        int pts = score(&cur[i], flags);
        const char *lv = pts >= 4 ? "high" : pts >= 2 ? "medium" : "low";
        if (j < 0) { printf("NEW %s %s flags=%s level=%s\n", cur[i].kind, cur[i].name, flags, lv); printed++; }
        else {
            int p = strcmp(base[j].path, cur[i].path) != 0, u = strcmp(base[j].user, cur[i].user) != 0;
            if (p || u) { printf("CHANGED %s %s changed=%s%s%s flags=%s level=%s\n", cur[i].kind, cur[i].name, p ? "path" : "", p && u ? "," : "", u ? "user" : "", flags, lv); printed++; }
        }
    }
    for (int i = 0; i < nb; i++) if (find(cur, nc, &base[i]) < 0) { printf("REMOVED %s %s\n", base[i].kind, base[i].name); printed++; }
    if (!printed) puts("no persistence changes");
    return 0;
}
