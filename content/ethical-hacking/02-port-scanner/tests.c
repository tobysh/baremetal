typedef struct { uint8_t kind[700]; int tries[700]; int calls; int outside; int lo, hi; int over; } Net;
// kinds: 0 closed, 1 open, 2 filtered, 3 open after 1 timeout, 4 closed after 2 timeouts, 5 open after 3 timeouts (too slow)
static int fake_probe(void *c, int port) {
    Net *n = c;
    n->calls++;
    if (port < n->lo || port > n->hi) { n->outside++; return PORT_TIMEOUT; }
    int i = port - n->lo;
    int t = n->tries[i]++;
    switch (n->kind[i]) {
    case 0: if (t) n->over++; return PORT_CLOSED;
    case 1: if (t) n->over++; return PORT_OPEN;
    case 2: return PORT_TIMEOUT;
    case 3: return t < 1 ? PORT_TIMEOUT : PORT_OPEN;
    case 4: return t < 2 ? PORT_TIMEOUT : PORT_CLOSED;
    default: return t < 3 ? PORT_TIMEOUT : PORT_OPEN;
    }
}
static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

static void run(uint32_t seed, int lo, int hi, int cap) {
    char name[96];
    Net n; memset(&n, 0, sizeof n);
    rs = seed; n.lo = lo; n.hi = hi;
    int wopen[700], wn = 0, wfilt = 0;
    for (int p = lo; p <= hi; p++) {
        uint32_t r = rnd() % 20; int k = r < 11 ? 0 : r < 14 ? 1 : r < 16 ? 2 : r < 17 ? 3 : r < 18 ? 4 : r < 19 ? 5 : 1;
        n.kind[p - lo] = (uint8_t)k;
        if (k == 1 || k == 3) { if (wn < cap) wopen[wn] = p; wn++; }
        if (k == 2 || k == 5) wfilt++;
    }
    int stored = wn < cap ? wn : cap;
    int open[700], filt = 99;
    int got = scan_ports(fake_probe, &n, lo, hi, open, cap, &filt);
    snprintf(name, sizeof name, "ports %d-%d: open ports found (cap %d)", lo, hi, cap);
    int ok = got == stored;
    for (int i = 0; ok && i < stored; i++) if (open[i] != wopen[i]) ok = 0;
    CHECK(name, ok, "returned %d, expected %d open ports (first ones: %d...)", got, stored, stored ? wopen[0] : 0);
    snprintf(name, sizeof name, "ports %d-%d: filtered count", lo, hi);
    CHECK(name, filt == wfilt, "filtered = %d, expected %d (retry timeouts twice, then give up)", filt, wfilt);
    snprintf(name, sizeof name, "ports %d-%d: no wasted or out-of-range probes", lo, hi);
    CHECK(name, n.outside == 0 && n.over == 0, "%d probes outside the range, %d repeats after a definite answer", n.outside, n.over);
    int maxcalls = 0, tot = 0; (void)maxcalls;
    for (int i = 0; i <= hi - lo; i++) { tot += n.tries[i]; if (n.tries[i] > 3) tot += 1000; }
    snprintf(name, sizeof name, "ports %d-%d: at most three attempts per port", lo, hi);
    CHECK(name, tot < 1000 && n.calls == tot, "a port was probed more than 3 times");
}

int main(void) {
    run(7, 1, 60, 100);
    run(99, 20, 140, 100);
    run(4242, 400, 560, 5);
    run(31337, 1, 1, 4);
    int open[8], filt = 5;
    Net n; memset(&n, 0, sizeof n); n.lo = 1; n.hi = 10;
    CHECK("lo below 1 is rejected", scan_ports(fake_probe, &n, 0, 10, open, 8, &filt) == -1 && n.calls == 0 && filt == 5, "must return -1 and not probe or touch *filtered");
    CHECK("hi above 65535 is rejected", scan_ports(fake_probe, &n, 65530, 65536, open, 8, &filt) == -1 && n.calls == 0, "must return -1 without probing");
    CHECK("lo above hi is rejected", scan_ports(fake_probe, &n, 9, 3, open, 8, &filt) == -1 && n.calls == 0, "must return -1 without probing");

    for (int t = 0; t < 4; t++) {
        rs = 500 + t;
        int on = 3 + rnd() % 8, an = 2 + rnd() % 5, o[16], a[8], want[16], wn = 0, out[16];
        for (int i = 0; i < on; i++) o[i] = 1 + (int)(rnd() % 3000);
        for (int i = 0; i < an; i++) a[i] = (i % 2 && on) ? o[rnd() % on] : 1 + (int)(rnd() % 3000);
        for (int i = 0; i < on; i++) { int ok = 0; for (int j = 0; j < an; j++) if (a[j] == o[i]) ok = 1; if (!ok) want[wn++] = o[i]; }
        int g = unexpected_open(o, on, a, an, out);
        int ok = g == wn;
        for (int i = 0; ok && i < wn; i++) if (out[i] != want[i]) ok = 0;
        char name[64]; snprintf(name, sizeof name, "unexpected_open, case %d", t + 1);
        CHECK(name, ok, "returned %d, expected %d unexpected ports", g, wn);
    }
    int none[1];
    CHECK("unexpected_open with nothing allowed", unexpected_open((int[]){22, 80}, 2, none, 0, (int[2]){0}) == 2, "with an empty allow-list every open port is unexpected");
    return 0;
}
