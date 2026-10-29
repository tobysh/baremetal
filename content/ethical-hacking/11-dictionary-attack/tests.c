static uint32_t ref_hash(const char *salt, const char *pw, int iters) {
    uint32_t h = 2166136261u;
    for (const char *p = salt; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    for (const char *p = pw; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    for (int i = 1; i < iters; i++) {
        uint32_t v = h; h = 2166136261u;
        for (int k = 0; k < 4; k++) { h ^= (v >> (8 * k)) & 255u; h *= 16777619u; }
        for (const char *p = salt; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    }
    return h;
}
static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

int main(void) {
    static const char *W[] = { "lantern", "orchid", "willow", "harbor", "copper", "meadow", "falcon", "ember", "quartz", "violet", "marble", "thistle", "juniper", "pebble", "saffron", "tundra", "walnut", "yonder", "zephyr", "bramble" };
    int bad = 0, badw = 0; char first[160] = "", firstw[160] = "";
    static const int ITERS[] = { 1, 3, 10, 64, 200 };
    for (int it = 0; it < 60; it++) {
        rs = 100 + it * 7;
        char salt[12]; snprintf(salt, sizeof salt, "s%u", rnd() % 100000);
        int iters = ITERS[rnd() % 5], n = 3 + rnd() % 17;
        const char *list[20]; for (int i = 0; i < n; i++) list[i] = W[(rnd() + i * 7) % 20];
        int missing = rnd() % 5 == 0;
        int pick = rnd() % n;
        uint32_t target = ref_hash(salt, missing ? "not-in-list" : list[pick], iters);
        int want = -1; for (int i = 0; i < n; i++) if (ref_hash(salt, list[i], iters) == target) { want = i; break; }
        long wwork = (want < 0 ? n : want + 1) * (long)iters, work = -5;
        int got = dict_attack(target, salt, iters, list, n, &work);
        if (got != want) { if (!bad) snprintf(first, sizeof first, "salt %s iters %d: expected index %d, got %d", salt, iters, want, got); bad++; }
        if (work != wwork) { if (!badw) snprintf(firstw, sizeof firstw, "salt %s iters %d: expected %ld rounds, got %ld", salt, iters, wwork, work); badw++; }
    }
    CHECK("attack finds the password", bad == 0, "%d of 60 wrong, e.g. %s", bad, first);
    CHECK("work counts rounds per attempted word", badw == 0, "%d of 60 wrong, e.g. %s", badw, firstw);
    {
        long w; const char *l[] = { "a" };
        CHECK("an empty word list finds nothing and costs nothing", dict_attack(1, "s", 5, l, 0, &w) == -1 && w == 0, "n = 0 gives -1 and work 0");
    }
    {
        // The same password, different salts: hashes differ; the same salt: equal.
        long w1, w2; const char *l[] = { "harbor", "copper", "ember" };
        uint32_t t = ref_hash("sA", "copper", 7);
        int a = dict_attack(t, "sA", 7, l, 3, &w1), b = dict_attack(t, "sB", 7, l, 3, &w2);
        CHECK("a different salt defeats the precomputed hash", a == 1 && w1 == 14 && b == -1 && w2 == 21, "right salt: index 1 after 14 rounds; wrong salt: -1 after 21 rounds. got %d/%ld and %d/%ld", a, w1, b, w2);
    }
    {
        int bads = 0; char fs[120] = "";
        for (int it = 0; it < 12; it++) {
            rs = 5000 + it;
            int n = 1 + rnd() % 12; uint32_t h[12];
            for (int i = 0; i < n; i++) h[i] = rnd() % 6;
            int want = 0; for (int i = 0; i < n; i++) { int c = 0; for (int j = 0; j < n; j++) if (h[j] == h[i]) c++; if (c > 1) want++; }
            int got = shared_hashes(h, n);
            if (got != want) { if (!bads) snprintf(fs, sizeof fs, "case %d: expected %d, got %d", it, want, got); bads++; }
        }
        uint32_t none[1] = { 9 };
        CHECK("password reuse is visible in unsalted hashes", bads == 0 && shared_hashes(none, 0) == 0 && shared_hashes(none, 1) == 0, "%d wrong, e.g. %s", bads, fs);
    }
    {
        struct { double rate; long words; double sec; long want; } t[] = {
            { 1e9, 1000000, 1.0, 1000 }, { 1e9, 1000000, 0.5, 500 }, { 100.0, 50, 1.0, 2 }, { 100.0, 50, 0.9, 2 }, { 100.0, 50, 0.01, 1 }, { 1e6, 1, 10.0, 10000000 },
            { 2500.0, 7, 3600.0, 1285715 }, { 1e10, 14000000, 86400.0, 61714286 }, { 10.0, 10, 5.0, 5 }, { 10.0, 10, 5.5, 6 } };
        int bad2 = 0; char f2[160] = "";
        for (size_t i = 0; i < sizeof t / sizeof t[0]; i++) {
            long g = iterations_needed(t[i].rate, t[i].words, t[i].sec);
            if (g != t[i].want) { if (!bad2) snprintf(f2, sizeof f2, "rate %.0f, %ld words, %.2f s: expected %ld, got %ld", t[i].rate, t[i].words, t[i].sec, t[i].want, g); bad2++; }
        }
        CHECK("iteration count for a target time", bad2 == 0, "%d wrong, e.g. %s", bad2, f2);
    }
    return 0;
}
