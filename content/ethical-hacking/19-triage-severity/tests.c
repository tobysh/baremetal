static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
static int cl(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int rscore(const Finding *f) {
    int i = cl(f->impact, 0, 3), e = cl(f->exploitability, 0, 2), x = cl(f->exposure, 0, 2), a = cl(f->asset_value, 1, 3);
    if (!i) return 0;
    int s = (i * 4 + e * 3 + x * 2) * a;
    if (f->exploited) s += 10;
    if (f->mitigated) s /= 2;
    return s;
}
static int rcmp(const Finding *x, const Finding *y) {
    int sx = rscore(x), sy = rscore(y); if (sx != sy) return sy - sx;
    int ex = cl(x->exposure, 0, 2), ey = cl(y->exposure, 0, 2); if (ex != ey) return ey - ex;
    return strcmp(x->id, y->id);
}
int main(void) {
    int bad = 0; char first[160] = "";
    for (int it = 0; it < 2000; it++) {
        rs = 17 + it * 5;
        Finding f; memset(&f, 0, sizeof f); snprintf(f.id, sizeof f.id, "F-%d", it);
        f.impact = (int)(rnd() % 6) - 1; f.exploitability = (int)(rnd() % 5) - 1; f.exposure = (int)(rnd() % 5) - 1; f.asset_value = (int)(rnd() % 6) - 1;
        f.exploited = rnd() % 2; f.mitigated = rnd() % 2;
        int w = rscore(&f), g = score_finding(&f);
        if (w != g) { if (!bad) snprintf(first, sizeof first, "impact %d exploit %d exposure %d asset %d exploited %d mitigated %d: expected %d, got %d", f.impact, f.exploitability, f.exposure, f.asset_value, f.exploited, f.mitigated, w, g); bad++; }
    }
    CHECK("scores follow the formula, with clamping", bad == 0, "%d of 2000 wrong, e.g. %s", bad, first);
    {
        Finding a = { "A", 3, 2, 2, 3, 1, 0 }, b = { "B", 0, 2, 2, 3, 1, 1 }, c = { "C", 1, 0, 0, 1, 0, 0 }, d = { "D", 3, 2, 2, 3, 1, 1 };
        CHECK("worked examples", score_finding(&a) == 3 * 4 * 0 + (12 + 6 + 4) * 3 + 10 && score_finding(&b) == 0 && score_finding(&c) == 4 && score_finding(&d) == 38, "A %d (expect 76), B %d (0, no impact), C %d (4), D %d (expect 38, halved after adding 10)", score_finding(&a), score_finding(&b), score_finding(&c), score_finding(&d));
    }
    {
        struct { int s; const char *n; } t[] = { { 100, "critical" }, { 40, "critical" }, { 39, "high" }, { 25, "high" }, { 24, "medium" }, { 12, "medium" }, { 11, "low" }, { 1, "low" }, { 0, "info" }, { -5, "info" } };
        int b2 = 0; char f2[80] = "";
        for (size_t i = 0; i < sizeof t / sizeof t[0]; i++) if (strcmp(severity_name(t[i].s), t[i].n)) { if (!b2) snprintf(f2, sizeof f2, "score %d should be %s, got %s", t[i].s, t[i].n, severity_name(t[i].s)); b2++; }
        CHECK("severity bands", b2 == 0, "%d wrong, e.g. %s", b2, f2);
    }
    for (int round = 0; round < 3; round++) {
        Finding list[60], want[60]; int n = 20 + round * 15;
        rs = 700 + round;
        for (int i = 0; i < n; i++) {
            memset(&list[i], 0, sizeof list[i]);
            snprintf(list[i].id, sizeof list[i].id, "%c-%02d", 'A' + (int)(rnd() % 5), (int)(rnd() % 40));
            list[i].impact = (int)(rnd() % 5) - 1 + (round ? 0 : 1); list[i].exploitability = (int)(rnd() % 4); list[i].exposure = (int)(rnd() % 4) - (round == 2 ? 0 : 0);
            list[i].asset_value = 1 + (int)(rnd() % 3); list[i].exploited = (rnd() % 4) == 0; list[i].mitigated = (rnd() % 3) == 0;
            want[i] = list[i];
        }
        for (int i = 1; i < n; i++) { Finding v = want[i]; int j = i; while (j && rcmp(&want[j - 1], &v) > 0) { want[j] = want[j - 1]; j--; } want[j] = v; }
        triage(list, n);
        int ok = 1, at = -1;
        for (int i = 0; i < n; i++) if (strcmp(list[i].id, want[i].id) || rscore(&list[i]) != rscore(&want[i]) || cl(list[i].exposure, 0, 2) != cl(want[i].exposure, 0, 2)) { ok = 0; at = i; break; }
        char name[60]; snprintf(name, sizeof name, "queue order, %d findings", n);
        CHECK(name, ok, "position %d: expected %s (score %d), got %s (score %d)", at, at >= 0 ? want[at].id : "", at >= 0 ? rscore(&want[at]) : 0, at >= 0 ? list[at].id : "", at >= 0 ? rscore(&list[at]) : 0);
    }
    {
        Finding one[1] = { { "X", 1, 1, 1, 1, 0, 0 } };
        triage(one, 1); triage(one, 0);
        CHECK("one or zero findings are fine", !strcmp(one[0].id, "X"), "sorting 0 or 1 items must not change anything");
    }
    return 0;
}
