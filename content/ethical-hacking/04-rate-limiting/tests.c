// Independent reference: a flat event list per key, no slots to reuse.
typedef struct { char key[32]; long ts; } RefEv;
typedef struct { RefEv ev[4000]; int n; int limit; long window; } Ref;
static int ref_live(Ref *r, const char *k, long now, long *oldest) {
    int c = 0; *oldest = 0;
    for (int i = 0; i < r->n; i++) if (!strcmp(r->ev[i].key, k) && r->ev[i].ts > now - r->window) { if (!c) *oldest = r->ev[i].ts; c++; }
    return c;
}
static int ref_keys_live(Ref *r, long now) {
    char seen[64][32]; int ns = 0;
    for (int i = 0; i < r->n; i++) if (r->ev[i].ts > now - r->window) {
        int f = 0; for (int j = 0; j < ns; j++) if (!strcmp(seen[j], r->ev[i].key)) f = 1;
        if (!f) strcpy(seen[ns++], r->ev[i].key);
    }
    return ns;
}
static int ref_allow(Ref *r, const char *k, long now) {
    long o; int c = ref_live(r, k, now, &o);
    if (c == 0 && ref_keys_live(r, now) >= 8) return 0;
    if (c >= r->limit) return 0;
    strcpy(r->ev[r->n].key, k); r->ev[r->n++].ts = now; return 1;
}
static long ref_retry(Ref *r, const char *k, long now) {
    long o; int c = ref_live(r, k, now, &o);
    return c >= r->limit ? o + r->window - now : 0;
}
static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

static void stream(uint32_t seed, int limit, long window, int nkeys, int maxgap, int steps, const char *label) {
    static Ref ref; static Limiter l;
    char keys[12][32];
    for (int i = 0; i < 12; i++) snprintf(keys[i], 32, "198.51.100.%d", 10 + i);
    memset(&ref, 0, sizeof ref); ref.limit = limit; ref.window = window;
    rl_init(&l, limit, window);
    rs = seed; long now = 100; int badA = 0, badR = 0; char fa[120] = "", fr[120] = "";
    for (int s = 0; s < steps; s++) {
        now += rnd() % (maxgap + 1);
        const char *k = keys[rnd() % nkeys];
        long wr = ref_retry(&ref, k, now), gr = rl_retry_after(&l, k, now);
        if (wr != gr) { if (!badR) snprintf(fr, sizeof fr, "step %d key %s t=%ld expected %ld, got %ld", s, k, now, wr, gr); badR++; }
        int wa = ref_allow(&ref, k, now), ga = rl_allow(&l, k, now);
        if (wa != ga) { if (!badA) snprintf(fa, sizeof fa, "step %d key %s t=%ld expected %d, got %d", s, k, now, wa, ga); badA++; }
    }
    char name[120];
    snprintf(name, sizeof name, "%s: allow decisions", label);
    CHECK(name, badA == 0, "%d wrong decisions, first: %s", badA, fa);
    snprintf(name, sizeof name, "%s: retry-after seconds", label);
    CHECK(name, badR == 0, "%d wrong values, first: %s", badR, fr);
}

int main(void) {
    stream(1, 3, 10, 3, 4, 400, "limit 3 per 10 s, 3 clients");
    stream(2, 5, 60, 4, 20, 500, "limit 5 per 60 s, 4 clients");
    stream(3, 1, 7, 2, 9, 300, "limit 1 per 7 s");
    stream(4, 16, 30, 5, 3, 600, "limit 16 per 30 s");
    stream(5, 4, 15, 12, 5, 800, "12 clients crowd an 8-slot table");

    // Boundary: a request exactly `window` seconds old no longer counts.
    {
        Limiter l; rl_init(&l, 2, 10);
        int a = rl_allow(&l, "x", 0), b = rl_allow(&l, "x", 1), c = rl_allow(&l, "x", 9), d = rl_allow(&l, "x", 10);
        CHECK("window edge is exclusive", a && b && !c && d, "allowed: %d %d %d %d, expected 1 1 0 1", a, b, c, d);
    }
    // Blocked requests are not recorded.
    {
        Limiter l; rl_init(&l, 2, 10);
        rl_allow(&l, "x", 0); rl_allow(&l, "x", 1);
        for (int t = 2; t < 10; t++) rl_allow(&l, "x", t);
        CHECK("hammering does not extend the block", rl_allow(&l, "x", 10) == 1, "after the first request expired a slot should be free even though the client kept trying");
    }
    // An attacker guessing once per second for 10 minutes against 5 per minute.
    {
        Limiter l; rl_init(&l, 5, 60);
        int ok = 0; for (long t = 0; t < 600; t++) ok += rl_allow(&l, "203.0.113.9", t);
        CHECK("guessing is throttled to the limit", ok == 50, "%d of 600 guesses got through, expected 50 (5 per minute)", ok);
    }
    // The table must not overflow memory.
    {
        Limiter l; rl_init(&l, 2, 100);
        char k[32]; int over = 0;
        for (int i = 0; i < 20; i++) { snprintf(k, 32, "client-%d", i); over += rl_allow(&l, k, i) ? 1 : 0; }
        CHECK("a full table refuses new keys instead of overflowing", over == 8, "%d keys were accepted, the table holds 8", over);
    }
    return 0;
}
