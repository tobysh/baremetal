typedef struct { char tok[17]; char user[24]; long created, last; int live; } RS;
typedef struct { RS s[256]; int n; } Ref;
static void r_purge(Ref *r, long now) { for (int i = 0; i < r->n; i++) if (r->s[i].live && (now - r->s[i].last > 300 || now - r->s[i].created > 3600)) r->s[i].live = 0; }
static RS *r_find(Ref *r, const char *t) { if (!t) return NULL; for (int i = 0; i < r->n; i++) if (r->s[i].live && !strcmp(r->s[i].tok, t)) return &r->s[i]; return NULL; }
static int r_live(Ref *r) { int c = 0; for (int i = 0; i < r->n; i++) c += r->s[i].live; return c; }
static const char *r_login(Ref *r, const char *user, long now, uint64_t rnd, const char *old) {
    r_purge(r, now);
    RS *o = r_find(r, old); if (o) o->live = 0;
    char tok[17]; snprintf(tok, 17, "%016llx", (unsigned long long)rnd);
    if (r_find(r, tok)) return NULL;
    if (r_live(r) >= 8) return NULL;
    RS *s = &r->s[r->n++]; strcpy(s->tok, tok); snprintf(s->user, sizeof s->user, "%s", user); s->created = s->last = now; s->live = 1;
    return s->tok;
}
static int r_check(Ref *r, const char *t, long now, char *u, size_t cap) {
    r_purge(r, now); RS *s = r_find(r, t); if (!s) return 0; s->last = now; if (cap) snprintf(u, cap, "%s", s->user); return 1;
}
static uint64_t st;
static uint64_t rnd64(void) { st = st * 6364136223846793005ULL + 1442695040888963407ULL; return st ^ (st >> 29); }

static void stream(uint64_t seed, int steps, int maxgap, int widegap, const char *label) {
    static SessionStore ss; static Ref ref; memset(&ref, 0, sizeof ref);
    ss_init(&ss); st = seed;
    static const char *users[] = { "alice", "bob", "carol", "dave-with-a-long-name-x", "eve" };
    static const char *junk[] = { "", "0000000000000001", "short", "ffffffffffffffff0", "zzzzzzzzzzzzzzzz" };
    char issued[64][17]; int ni = 0;
    long now = 1000; int bad = 0; char first[200] = "";
    for (int step = 0; step < steps; step++) {
        now += (long)(rnd64() % (uint64_t)(maxgap + 1));
        if (widegap && rnd64() % 25 == 0) now += 3000;
        int op = (int)(rnd64() % 10);
        const char *tok = NULL;
        if (ni && rnd64() % 4) tok = issued[rnd64() % ni]; else tok = junk[rnd64() % 5];
        if (op < 4) {
            const char *u = users[rnd64() % 5];
            uint64_t rv = rnd64();
            if (ni && rnd64() % 12 == 0) { char *e; rv = strtoull(issued[rnd64() % ni], &e, 16); }
            const char *old = (rnd64() % 3) ? NULL : tok;
            char oldcopy[17]; if (old) { snprintf(oldcopy, 17, "%s", old); old = oldcopy; }
            const char *w = r_login(&ref, u, now, rv, old), *g = ss_login(&ss, u, now, rv, old);
            if ((w == NULL) != (g == NULL) || (w && strcmp(w, g))) { if (!bad) snprintf(first, sizeof first, "step %d login(%s, rnd %llx): expected %s, got %s", step, u, (unsigned long long)rv, w ? w : "NULL", g ? g : "NULL"); bad++; }
            if (w && ni < 64) snprintf(issued[ni++], 17, "%s", w);
        } else if (op < 9) {
            char wu[24] = "", gu[24] = "";
            int w = r_check(&ref, tok, now, wu, sizeof wu), g = ss_check(&ss, tok, now, gu, sizeof gu);
            if (w != g || (w && strcmp(wu, gu))) { if (!bad) snprintf(first, sizeof first, "step %d check(%s) at t=%ld: expected %d, got %d", step, tok, now, w, g); bad++; }
        } else {
            RS *s = r_find(&ref, tok); r_purge(&ref, now); s = r_find(&ref, tok); if (s) s->live = 0;
            ss_logout(&ss, tok);
        }
    }
    char name[120]; snprintf(name, sizeof name, "%s", label);
    CHECK(name, bad == 0, "%d wrong results, first: %s", bad, first);
}

int main(void) {
    stream(1, 400, 40, 0, "random traffic: tokens, idle expiry, logout");
    stream(2, 600, 120, 1, "random traffic with long pauses (absolute lifetime)");
    stream(3, 500, 8, 0, "busy store near its 8-session limit");
    {
        SessionStore ss; ss_init(&ss);
        const char *a = ss_login(&ss, "alice", 0, 0xdeadbeefULL, NULL);
        CHECK("token is the 16-digit hex of rnd", a && !strcmp(a, "00000000deadbeef"), "got %s", a ? a : "NULL");
        const char *b = ss_login(&ss, "bob", 0, 0x1ULL, NULL);
        CHECK("tokens are not a counter", b && !strcmp(b, "0000000000000001") && strcmp(a, "00000000deadbeef") == 0, "token must come from rnd, got %s", b ? b : "NULL");
    }
    {
        SessionStore ss; ss_init(&ss); char u[24];
        ss_login(&ss, "alice", 0, 0xabcdef0123456789ULL, NULL);
        CHECK("a prefix of a token does not match", !ss_check(&ss, "abcdef01", 1, u, sizeof u) && !ss_check(&ss, "abcdef0123456789ff", 1, u, sizeof u) && ss_check(&ss, "abcdef0123456789", 1, u, sizeof u), "only the whole token matches");
        CHECK("NULL token is invalid", !ss_check(&ss, NULL, 1, u, sizeof u), "NULL must not match");
    }
    {
        SessionStore ss; ss_init(&ss); char u[24];
        char pre[17]; snprintf(pre, 17, "%s", ss_login(&ss, "anon", 0, 0x1111111111111111ULL, NULL));
        const char *t = ss_login(&ss, "victim", 5, 0x2222222222222222ULL, pre);
        CHECK("login invalidates the pre-login token (fixation)", t && !ss_check(&ss, pre, 6, u, sizeof u) && ss_check(&ss, t, 6, u, sizeof u) && !strcmp(u, "victim"), "the old token must stop working at login");
    }
    {
        SessionStore ss; ss_init(&ss); char u[24];
        ss_login(&ss, "alice", 0, 0xaaULL, NULL);
        CHECK("idle for 300 s is still alive", ss_check(&ss, "00000000000000aa", 300, u, sizeof u), "exactly 300 s idle is allowed");
        CHECK("idle for 301 s is dead", !ss_check(&ss, "00000000000000aa", 602, u, sizeof u), "301 s idle must expire");
        ss_login(&ss, "bob", 1000, 0xbbULL, NULL);
        int ok = 1; for (long t = 1200; t <= 4400; t += 200) ok &= ss_check(&ss, "00000000000000bb", t, u, sizeof u);
        CHECK("an active session lasts until its absolute lifetime", ok && ss_check(&ss, "00000000000000bb", 4600, u, sizeof u) && !ss_check(&ss, "00000000000000bb", 4601, u, sizeof u), "age 3600 is fine, 3601 is not");
        ss_login(&ss, "carol", 5000, 0xccULL, NULL);
        CHECK("a collision does not overwrite another user", ss_login(&ss, "mallory", 5001, 0xccULL, NULL) == NULL && ss_check(&ss, "00000000000000cc", 5002, u, sizeof u) && !strcmp(u, "carol"), "return NULL and keep carol's session");
        ss_logout(&ss, "00000000000000cc");
        CHECK("logout ends the session on the server", !ss_check(&ss, "00000000000000cc", 5003, u, sizeof u), "token still works after logout");
    }
    return 0;
}
