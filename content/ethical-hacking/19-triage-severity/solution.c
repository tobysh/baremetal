#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { char id[16]; int impact, exploitability, exposure, asset_value, exploited, mitigated; } Finding;

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

int score_finding(const Finding *f) {
    int impact = clamp(f->impact, 0, 3), ex = clamp(f->exploitability, 0, 2), exp = clamp(f->exposure, 0, 2), av = clamp(f->asset_value, 1, 3);
    if (impact == 0) return 0;
    int s = (impact * 4 + ex * 3 + exp * 2) * av;
    if (f->exploited) s += 10;
    if (f->mitigated) s /= 2;
    return s;
}

const char *severity_name(int score) {
    return score >= 40 ? "critical" : score >= 25 ? "high" : score >= 12 ? "medium" : score >= 1 ? "low" : "info";
}

static int cmp(const void *a, const void *b) {
    const Finding *x = a, *y = b;
    int sx = score_finding(x), sy = score_finding(y);
    if (sx != sy) return sy - sx;
    int ex = clamp(x->exposure, 0, 2), ey = clamp(y->exposure, 0, 2);
    if (ex != ey) return ey - ex;
    return strcmp(x->id, y->id);
}

void triage(Finding *list, int n) { qsort(list, (size_t)n, sizeof *list, cmp); }

int main(void) {
    Finding f = { "F-1", 3, 2, 2, 3, 1, 0 };
    printf("%d %s\n", score_finding(&f), severity_name(score_finding(&f)));
    return 0;
}
