typedef struct { uint8_t kind[400]; int tries[400]; const char *ban[400]; int lo, calls, bcalls, bad; } Net;
static int fprobe(void *c, int port) {
    Net *n = c; n->calls++;
    if (port < n->lo || port >= n->lo + 400) { n->bad++; return PORT_TIMEOUT; }
    int i = port - n->lo, t = n->tries[i]++;
    switch (n->kind[i]) {
    case 0: if (t) n->bad++; return PORT_CLOSED;
    case 1: if (t) n->bad++; return PORT_OPEN;
    case 2: return PORT_TIMEOUT;
    default: return t < 2 ? PORT_TIMEOUT : PORT_OPEN;
    }
}
static const char *fbanner(void *c, int port) { Net *n = c; n->bcalls++; return n->ban[port - n->lo]; }
static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

static void run(uint32_t seed, int lo, int hi, int cap) {
    static Net n; memset(&n, 0, sizeof n);
    static const char *B[] = { "toyhttpd/1.1", "toyhttpd/1.2", "toyhttpd/1.10", "quillsh/2.9", "quillsh/3.0.1", "quillsh", "mailtoy/2.4", "mailtoy", "finchftpd/0.9", NULL, NULL, "web/1.0" };
    static const Advisory adv[] = { { "toyhttpd", "1.2" }, { "quillsh", "3.0" }, { "mailtoy", "2.5" } };
    rs = seed; n.lo = lo;
    int allowed[8], na = 2 + rnd() % 5;
    for (int i = 0; i < na; i++) allowed[i] = lo + rnd() % (hi - lo + 1);
    Finding want[400]; int wn = 0;
    for (int p = lo; p <= hi; p++) {
        uint32_t r = rnd() % 10; int k = r < 6 ? 0 : r < 8 ? 1 : r < 9 ? 2 : 3;
        n.kind[p - lo] = (uint8_t)k; n.ban[p - lo] = B[rnd() % 12];
        if (k == 1 || k == 3) {
            int ok = 0; for (int i = 0; i < na; i++) if (allowed[i] == p) ok = 1;
            if (!ok) { want[wn].port = p; want[wn].kind = F_UNEXPECTED; want[wn++].adv = -1; }
            const char *b = n.ban[p - lo];
            if (b) {
                const char *sl = strchr(b, '/'); size_t pl = sl ? (size_t)(sl - b) : strlen(b);
                for (int i = 0; i < 3; i++) if (strlen(adv[i].product) == pl && !strncmp(adv[i].product, b, pl)) {
                    if (!sl) { want[wn].port = p; want[wn].kind = F_UNVERIFIED; want[wn++].adv = i; }
                    else if (ver_cmp(sl + 1, adv[i].fixed_in) < 0) { want[wn].port = p; want[wn].kind = F_OUTDATED; want[wn++].adv = i; }
                    break;
                }
            }
        }
    }
    HostApi h = { fprobe, fbanner, &n };
    Finding out[400]; for (int i = 0; i < 400; i++) out[i].port = -7;
    int got = audit_host(&h, lo, hi, allowed, na, adv, 3, out, cap);
    int exp = wn < cap ? wn : cap, ok = got == exp;
    char d[160] = "";
    for (int i = 0; ok && i < exp; i++) if (out[i].port != want[i].port || out[i].kind != want[i].kind || out[i].adv != want[i].adv) { ok = 0; snprintf(d, sizeof d, "finding %d: expected port %d kind %d adv %d, got port %d kind %d adv %d", i, want[i].port, want[i].kind, want[i].adv, out[i].port, out[i].kind, out[i].adv); }
    if (got != exp) snprintf(d, sizeof d, "returned %d findings, expected %d", got, exp);
    char name[100];
    snprintf(name, sizeof name, "ports %d-%d, cap %d: findings", lo, hi, cap);
    CHECK(name, ok, "%s", d);
    int overflow = 0; for (int i = exp; i < 400; i++) if (out[i].port != -7) overflow++;
    snprintf(name, sizeof name, "ports %d-%d, cap %d: never writes past cap", lo, hi, cap);
    CHECK(name, !overflow, "%d findings were written beyond the cap", overflow);
    snprintf(name, sizeof name, "ports %d-%d, cap %d: probes politely", lo, hi, cap);
    int nopen = 0; for (int i = 0; i <= hi - lo; i++) if (n.kind[i] == 1 || n.kind[i] == 3) nopen++;
    CHECK(name, n.bad == 0 && n.bcalls == nopen, "%d bad probes (out of range or repeated after an answer), %d banner reads for %d open ports", n.bad, n.bcalls, nopen);
}

int main(void) {
    run(11, 1, 120, 100);
    run(12, 300, 640, 100);
    run(13, 50, 250, 3);
    run(14, 8000, 8100, 100);
    HostApi h = { fprobe, fbanner, NULL };
    Finding f[2];
    CHECK("invalid ranges return -1", audit_host(&h, 0, 5, NULL, 0, NULL, 0, f, 2) == -1 && audit_host(&h, 9, 3, NULL, 0, NULL, 0, f, 2) == -1 && audit_host(&h, 1, 70000, NULL, 0, NULL, 0, f, 2) == -1, "lo < 1, hi > 65535 and lo > hi are all invalid");
    return 0;
}
