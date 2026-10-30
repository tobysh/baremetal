#define IP(a,b,c,d) (((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(uint32_t)(d))
typedef struct { int calls; char last[100]; } Dns;
typedef struct { const char *host; int ok; uint32_t ip; } Rec;
static const Rec RECS[] = {
    { "api.cdn-example.test", 1, IP(203,0,113,10) }, { "img.cdn-example.test", 1, IP(10,1,2,3) }, { "gone.cdn-example.test", 0, 0 },
    { "meta.cdn-example.test", 1, IP(169,254,169,254) }, { "loop.cdn-example.test", 1, IP(127,0,0,1) }, { "evil.test", 1, IP(198,51,100,9) },
    { "p1.cdn-example.test", 1, IP(172,15,255,255) }, { "p2.cdn-example.test", 1, IP(172,16,0,0) }, { "p3.cdn-example.test", 1, IP(172,31,255,255) },
    { "p4.cdn-example.test", 1, IP(172,32,0,0) }, { "p5.cdn-example.test", 1, IP(100,64,0,1) }, { "p6.cdn-example.test", 1, IP(100,128,0,1) },
    { "p7.cdn-example.test", 1, IP(0,1,2,3) }, { "p8.cdn-example.test", 1, IP(224,0,0,1) }, { "p9.cdn-example.test", 1, IP(192,168,0,1) },
    { "p10.cdn-example.test", 1, IP(192,169,0,1) }, { "p11.cdn-example.test", 1, IP(127,255,255,255) }, { "p12.cdn-example.test", 1, IP(169,253,1,1) },
    { "p13.cdn-example.test", 1, IP(10,0,0,0) }, { "p14.cdn-example.test", 1, IP(11,0,0,1) }, { "p15.cdn-example.test", 1, IP(255,255,255,255) },
    { "p16.cdn-example.test", 1, IP(100,127,255,255) }, { "p17.cdn-example.test", 1, IP(223,255,255,255) }, { "p18.cdn-example.test", 1, IP(169,254,0,1) },
    { "p19.cdn-example.test", 1, IP(192,168,255,255) }, { "p20.cdn-example.test", 1, IP(172,20,1,1) }, { "p21.cdn-example.test", 1, IP(0,255,255,255) },
    { "p22.cdn-example.test", 1, IP(1,0,0,1) },
};
static uint32_t dns(void *c, const char *host, int *ok) {
    Dns *d = c; d->calls++; snprintf(d->last, sizeof d->last, "%s", host);
    for (size_t i = 0; i < sizeof RECS / sizeof RECS[0]; i++) if (!strcmp(RECS[i].host, host)) { *ok = RECS[i].ok; return RECS[i].ip; }
    *ok = 0; return 0;
}
static const char *ALLOW[] = { "*.cdn-example.test" };
static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

static int internal_ref(uint32_t ip) {
    unsigned a = ip >> 24, b = (ip >> 16) & 255;
    return a == 0 || a == 10 || a == 127 || a >= 224 || (a == 100 && b >= 64 && b <= 127) || (a == 169 && b == 254) || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168);
}

int main(void) {
    {
        const char *schemes[] = { "http", "https", "HTTP", "Https", "ftp", "file", "gopher" };
        const char *users[] = { "", "", "", "user@", "a:b@" };
        struct { const char *h; int allowed; int dnsk; } hosts[] = {
            { "api.cdn-example.test", 1, 0 }, { "API.CDN-Example.test", 1, 0 }, { "api.cdn-example.test.", 1, 0 }, { "img.cdn-example.test", 1, 6 }, { "gone.cdn-example.test", 1, 7 },
            { "meta.cdn-example.test", 1, 6 }, { "loop.cdn-example.test", 1, 6 }, { "evil.test", 0, 0 }, { "cdn-example.test.evil.test", 0, 0 }, { "xcdn-example.test", 0, 0 },
            { "cdn-example.test", 0, 0 }, { "127.0.0.1", 0, 0 }, { "2130706433", 0, 0 }, { "evil-cdn-example.test", 0, 0 } };
        const char *ports[] = { "", ":80", ":443", ":8080", ":22" };
        const char *paths[] = { "", "/", "/x?y=1", "?q=1", "#f" };
        int bad = 0, tot = 0, dnsbad = 0; char first[200] = "";
        for (int it = 0; it < 1500; it++) {
            rs = 77 + it * 13;
            int si = rnd() % 7, ui = rnd() % 5, hi = rnd() % 14, pi = rnd() % 5, pa = rnd() % 5;
            char url[300]; snprintf(url, sizeof url, "%s://%s%s%s%s", schemes[si], users[ui], hosts[hi].h, ports[pi], paths[pa]);
            int https = (si == 1 || si == 3), is_http = si < 4;
            int want;
            if (!is_http) want = URL_SCHEME;
            else if (users[ui][0]) want = URL_CREDENTIALS;
            else if (ports[pi][0] && atoi(ports[pi] + 1) != (https ? 443 : 80)) want = URL_PORT;
            else if (!hosts[hi].allowed) want = URL_NOT_ALLOWED;
            else want = hosts[hi].dnsk ? hosts[hi].dnsk : URL_OK;
            Dns d = { 0, "" }; uint32_t pin = 0xdeadbeef;
            int got = url_policy(url, dns, &d, ALLOW, 1, &pin);
            tot++;
            if (got != want) { if (!bad) snprintf(first, sizeof first, "%s expected %d, got %d", url, want, got); bad++; }
            int wantcalls = (want == URL_OK || want == URL_INTERNAL || want == URL_NO_ADDRESS) ? 1 : 0;
            if (d.calls != wantcalls) dnsbad++;
            if (want != URL_OK && pin != 0xdeadbeef) dnsbad++;
            if (want == URL_OK && pin != IP(203,0,113,10)) dnsbad++;
        }
        CHECK("URL policy gives the first failing reason", bad == 0, "%d of %d generated URLs wrong, e.g. %s", bad, tot, first);
        CHECK("resolver is used once, only when needed, and the address is pinned", dnsbad == 0, "%d URLs resolved the wrong number of times or mishandled *pinned", dnsbad);
    }
    {
        int bad = 0; char first[120] = "";
        for (size_t i = 0; i < sizeof RECS / sizeof RECS[0]; i++) {
            if (strncmp(RECS[i].host, "p", 1) || !RECS[i].ok) continue;
            char url[100]; snprintf(url, sizeof url, "https://%s/a", RECS[i].host);
            Dns d = { 0, "" }; uint32_t pin = 0;
            int got = url_policy(url, dns, &d, ALLOW, 1, &pin), want = internal_ref(RECS[i].ip) ? URL_INTERNAL : URL_OK;
            if (got != want || (want == URL_OK && pin != RECS[i].ip)) { if (!bad) snprintf(first, sizeof first, "%s (%u.%u.%u.%u): expected %d, got %d", RECS[i].host, RECS[i].ip >> 24, (RECS[i].ip >> 16) & 255, (RECS[i].ip >> 8) & 255, RECS[i].ip & 255, want, got); bad++; }
        }
        CHECK("internal address ranges, with boundaries", bad == 0, "%d wrong, e.g. %s", bad, first);
    }
    {
        struct { const char *u; int want; } t[] = {
            { "api.cdn-example.test/path", URL_MALFORMED }, { "http://", URL_MALFORMED }, { "http:///path", URL_MALFORMED }, { "http://api.cdn-example.test:/", URL_MALFORMED },
            { "http://api.cdn-example.test:80x/", URL_MALFORMED }, { "http://api.cdn-example.test\\@evil.test/", URL_MALFORMED }, { "http://exa mple.test/", URL_MALFORMED },
            { "http://api.cdn-example.test@evil.test/", URL_CREDENTIALS }, { "http://evil.test@api.cdn-example.test/", URL_CREDENTIALS }, { "http://api.cdn-example.test:80/", URL_OK },
            { "https://api.cdn-example.test:443/", URL_OK }, { "https://api.cdn-example.test:80/", URL_PORT }, { "http://api.cdn-example.test:443/", URL_PORT }, { "http://:80/", URL_MALFORMED },
            { "http://api.cdn-example.test?x=@", URL_OK }, { "http://api.cdn-example.test#@evil.test", URL_OK }, { "ftp://evil.test", URL_SCHEME }, { "://x", URL_SCHEME } };
        int bad = 0; char first[160] = "";
        for (size_t i = 0; i < sizeof t / sizeof t[0]; i++) { Dns d = { 0, "" }; int got = url_policy(t[i].u, dns, &d, ALLOW, 1, NULL); if (got != t[i].want) { if (!bad) snprintf(first, sizeof first, "[%s] expected %d, got %d", t[i].u, t[i].want, got); bad++; } }
        CHECK("tricky URLs", bad == 0, "%d wrong, e.g. %s", bad, first);
    }
    {
        Dns d = { 0, "" }; uint32_t pin = 0;
        int r1 = redirect_policy("/next/page?x=1", dns, &d, ALLOW, 1, &pin);
        int r2 = redirect_policy("//evil.test/x", dns, &d, ALLOW, 1, &pin);
        int r3 = redirect_policy("http://img.cdn-example.test/x", dns, &d, ALLOW, 1, &pin);
        int r4 = redirect_policy("https://api.cdn-example.test/x", dns, &d, ALLOW, 1, &pin);
        int r5 = redirect_policy("http://evil.test/", dns, &d, ALLOW, 1, &pin);
        CHECK("redirects are checked like new requests", r1 == 0 && r2 == URL_MALFORMED && r3 == URL_INTERNAL && r4 == 0 && r5 == URL_NOT_ALLOWED && d.calls == 2, "got %d %d %d %d %d (expected 0 2 6 0 4) and %d lookups (expected 2)", r1, r2, r3, r4, r5, d.calls);
    }
    {
        const char *al[] = { "static.cdn-example.test", "*.files.test" };
        Dns d = { 0, "" };
        int a = url_policy("http://static.cdn-example.test/", dns, &d, al, 2, NULL), b = url_policy("http://x.files.test/", dns, &d, al, 2, NULL), c = url_policy("http://files.test/", dns, &d, al, 2, NULL);
        CHECK("allow-list supports exact and wildcard entries", a == URL_NO_ADDRESS && b == URL_NO_ADDRESS && c == URL_NOT_ALLOWED, "got %d %d %d, expected 7 7 4 (allowed names that do not resolve in the test DNS give 7)", a, b, c);
    }
    return 0;
}
