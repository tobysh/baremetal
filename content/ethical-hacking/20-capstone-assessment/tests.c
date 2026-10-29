typedef struct { int lo, hi, calls[32], banners[32], outside, docs, fixed; DocStore store; } Lab;
static int probe_lab(void *ctx, int port) {
    Lab *l = ctx;
    if (port < l->lo || port > l->hi) { l->outside++; return PORT_CLOSED; }
    int i = port - l->lo, n = ++l->calls[i];
    if (i == 0) return n < 3 ? PORT_TIMEOUT : PORT_OPEN;
    if (i == 1 || i == 2 || i == 4) return PORT_OPEN;
    if (i == 3) return PORT_TIMEOUT;
    return PORT_CLOSED;
}
static const char *banner_lab(void *ctx, int port) {
    Lab *l = ctx; int i = port - l->lo;
    if (i < 0 || i >= 32) { l->outside++; return NULL; }
    l->banners[i]++;
    return i == 0 ? "demo/1.9" : i == 1 ? "demo/2.10" : i == 2 ? "demo" : NULL;
}
static int doc_lab(void *ctx, int caller, int id, char *out, size_t cap) {
    Lab *l = ctx; l->docs++;
    if (l->fixed) return doc_fixed(&l->store, caller, id, out, cap);
    if (cap) snprintf(out, cap, "private");
    return 200;
}
int main(void) {
    for (int seed = 0; seed < 8; seed++) {
        Lab l = { .lo = 8000 + seed * 11, .hi = 8005 + seed * 11 };
        Policy p = { .authorized = 1, .scope_lo = l.lo, .scope_hi = l.hi,
            .nallowed = 1, .nadv = 1, .tester_uid = 7, .ndocs = 2 };
        p.allowed[0] = l.lo + 1; p.adv[0] = (Advisory){"demo", "2.9"};
        p.docs[0].id = 10; p.docs[0].owner = 7;
        p.docs[1].id = 100 + seed; p.docs[1].owner = 9;
        Target t = { probe_lab, banner_lab, doc_lab, &l }; Report r = {0};
        p.authorized = 0;
        CHECK("authorization blocks callbacks", assess(&t, &p, &r) == -2 && !l.calls[0] && !l.docs, "unauthorized callbacks");
        p.authorized = 1; p.scope_lo = 0;
        CHECK("invalid scope blocks callbacks", assess(&t, &p, &r) == -1 && !l.calls[0], "invalid range scanned");
        p.scope_lo = l.lo;
        int n = assess(&t, &p, &r);
        CHECK("combined assessment", n == 6 && r.n == 6, "expected six findings, got %d", n);
        CHECK("bounded retries and scope", !l.outside && l.calls[0] == 3 && l.calls[3] == 3 && l.calls[1] == 1 && l.calls[5] == 1, "probe contract");
        CHECK("open banners only once", l.banners[0] == 1 && l.banners[1] == 1 && l.banners[2] == 1 && !l.banners[3] && l.banners[4] == 1 && !l.banners[5], "banner contract");
        CHECK("access finding first", r.f[0].kind == K_IDOR && r.f[0].severity == 4 && l.docs == 1, "IDOR or owned-document handling");
        CHECK("numeric versions and stable ordering", r.f[1].kind == K_OUTDATED && r.f[1].port == l.lo && r.f[2].port == l.lo && r.f[3].port == l.lo+2 && r.f[4].port == l.lo+4 && r.f[5].kind == K_UNVERIFIED, "finding order");
        for (int i = 0; i < r.n && i < 16; i++) {
            char number[32]; snprintf(number, sizeof number, "%d", r.f[i].port ? r.f[i].port : p.docs[1].id);
            CHECK("actionable finding", r.f[i].title[0] && r.f[i].root_cause[0] && r.f[i].remediation[0] && strstr(r.f[i].evidence, number), "missing explanation or evidence");
        }
        memset(l.calls, 0, sizeof l.calls); memset(l.banners, 0, sizeof l.banners);
        l.fixed = 1; l.store.n = 1; l.store.docs[0] = (Doc){ .id = p.docs[1].id, .owner = 9, .text = "secret" };
        CHECK("repair removes IDOR", assess(&t, &p, &r) == 5, "fixed host still leaks");
        char out[16] = "untouched";
        CHECK("nonowner denied without writes", doc_fixed(&l.store, 7, p.docs[1].id, out, sizeof out) == 404 && !strcmp(out,"untouched"), "access bypass");
        CHECK("owner receives truncated text", doc_fixed(&l.store, 9, p.docs[1].id, out, 4) == 200 && !strcmp(out,"sec"), "copy bounds");
        CHECK("zero capacity", doc_fixed(&l.store, 9, p.docs[1].id, NULL, 0) == 200, "zero cap");
        CHECK("missing document", doc_fixed(&l.store, 9, -1, out, sizeof out) == 404, "unknown id");
    }
    Report r = {.n = 1}; r.f[0] = (Finding){.severity=3, .title="Title\nnext", .root_cause="Cause\rnext", .evidence="proof", .remediation="fix"};
    const char *want = "REPORT: 1 finding\n\nFINDING: Title next\nSEVERITY: High\nROOT CAUSE: Cause next\nEVIDENCE:\n- proof\nREMEDIATION: fix\n";
    char out[12000]; int n = report_text(&r, out, sizeof out);
    CHECK("report structure and flattened values", n == (int)strlen(want) && !strcmp(out,want), "report mismatch");
    CHECK("exact capacity needs NUL", report_text(&r,out,strlen(want)) == -1 && out[0] == 0, "overflow handling");
    CHECK("zero report capacity", report_text(&r,NULL,0) == -1, "zero capacity");
    r.n = 0; CHECK("empty report", report_text(&r,out,sizeof out) == 19 && !strcmp(out,"REPORT: 0 findings\n"), "empty report");
    return 0;
}
