static uint32_t rs = 1;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

static int ref_scope(const char *e, const char *t) {
    if (e[0] == '*' && e[1] == '.') { size_t el = strlen(e) - 1, tl = strlen(t); return tl > el && strcmp(t + tl - el, e + 1) == 0; }
    return strcmp(e, t) == 0;
}
static int ref(const Roe *r, const char *t, int day, const char *tech) {
    if (!r->owner_signed) return 1;
    int ok = 0;
    for (int i = 0; i < r->nscope; i++) if (ref_scope(r->scope[i], t)) ok = 1;
    if (!ok) return 2;
    if (day < r->start_day || day > r->end_day) return 3;
    for (int i = 0; i < r->nforbidden; i++) if (!strcmp(r->forbidden[i], tech)) return 4;
    return 0;
}

int main(void) {
    static const int want[8] = { 0, 1, 0, 1, 0, 1, 1, 0 };
    static const char *why[8] = {
        "verbal approval is not permission", "the scope list is also a fence", "never go further than you were allowed to confirm",
        "you own it, so you may test it", "intent is not a defence, permission is", "that is the definition", "stop and ask", "the signer must control the system" };
    for (int i = 0; i < 8; i++) {
        char name[64];
        snprintf(name, sizeof name, "quiz statement %d", i);
        CHECK(name, quiz_answer(i) == want[i], "wrong answer. Hint: %s", why[i]);
    }

    static const char *hosts[] = { "web.lab.test", "db.lab.test", "lab.test", "xlab.test", "a.b.lab.test", "pay.vendor.test", "www.example.test", "web.lab.test.evil.test", "vendor.test" };
    static const char *scopes[] = { "*.lab.test", "web.lab.test", "www.example.test", "*.example.test", "vendor.test" };
    static const char *techs[] = { "portscan", "dos", "phish", "sqli", "bruteforce" };
    int bad[5] = {0}; char first[5][200] = {{0}};
    for (int it = 0; it < 600; it++) {
        rs = 12345 + it * 7919u;
        Roe r; memset(&r, 0, sizeof r);
        r.owner_signed = (rnd() % 5) != 0;
        r.start_day = rnd() % 20; r.end_day = r.start_day + rnd() % 10;
        r.nscope = rnd() % 4;
        for (int i = 0; i < r.nscope; i++) r.scope[i] = scopes[rnd() % 5];
        r.nforbidden = rnd() % 3;
        for (int i = 0; i < r.nforbidden; i++) r.forbidden[i] = techs[rnd() % 5];
        const char *t = hosts[rnd() % 9];
        int day = (int)(rnd() % 34) - 2;
        const char *tech = techs[rnd() % 5];
        int w = ref(&r, t, day, tech), g = may_proceed(&r, t, day, tech);
        if (g != w) { bad[w]++; if (!first[w][0]) snprintf(first[w], sizeof first[w], "target %s day %d technique %s (signed %d, window %d-%d): expected %d, got %d", t, day, tech, r.owner_signed, r.start_day, r.end_day, w, g); }
    }
    static const char *nm[5] = { "engagements in order are approved", "unsigned engagements are stopped first", "out-of-scope targets are stopped", "days outside the window are stopped", "forbidden techniques are stopped" };
    for (int k = 0; k < 5; k++) CHECK(nm[k], bad[k] == 0, "%d random engagements wrong, e.g. %s", bad[k], first[k]);

    Roe a = { 1, 5, 9, { "*.lab.test" }, 1, { "dos" }, 1 };
    CHECK("wildcard does not match the bare domain", may_proceed(&a, "lab.test", 6, "portscan") == STOP_OUT_OF_SCOPE, "lab.test is not *.lab.test");
    CHECK("wildcard does not match a look-alike", may_proceed(&a, "xlab.test", 6, "portscan") == STOP_OUT_OF_SCOPE, "xlab.test must be out of scope");
    CHECK("window ends are inclusive", may_proceed(&a, "a.lab.test", 5, "x") == 0 && may_proceed(&a, "a.lab.test", 9, "x") == 0 && may_proceed(&a, "a.lab.test", 10, "x") == STOP_OUTSIDE_WINDOW, "days 5 and 9 are inside, 10 is outside");
    Roe e = { 1, 0, 100, { 0 }, 0, { 0 }, 0 };
    CHECK("an empty scope allows nothing", may_proceed(&e, "web.lab.test", 1, "x") == STOP_OUT_OF_SCOPE, "no scope entries means nothing is in scope");
    return 0;
}
