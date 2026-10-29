static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

typedef struct { int proto; char prod[24]; char ver[16]; char banner[256]; } Gen;
static const char *PRODS[4][3] = { { "quillsh", "wrenssh", "tarnsh" }, { "mailtoy", "postlark", "relayfin" }, { "finchftpd", "pipitftp", "larkftpd" }, { "toyhttpd", "minihttpd", "reedweb" } };
static void gen(Gen *g, int proto, int hide) {
    g->proto = proto;
    snprintf(g->prod, sizeof g->prod, "%s", PRODS[proto - 1][rnd() % 3]);
    int a = rnd() % 12, b = rnd() % 12, c = rnd() % 4, comps = 2 + rnd() % 2;
    if (comps == 2) snprintf(g->ver, sizeof g->ver, "%d.%d", a, b); else snprintf(g->ver, sizeof g->ver, "%d.%d.%d", a, b, c);
    const char *v = hide ? "" : g->ver;
    if (hide) g->ver[0] = '\0';
    switch (proto) {
    case PROTO_SSH:
        if (hide) snprintf(g->banner, sizeof g->banner, "SSH-%s-%s\r\n", rnd() % 2 ? "2.0" : "1.99", g->prod);
        else snprintf(g->banner, sizeof g->banner, "SSH-%s-%s_%s %s\r\n", rnd() % 2 ? "2.0" : "1.99", g->prod, v, rnd() % 2 ? "Ubuntu-fake" : "build9");
        break;
    case PROTO_SMTP:
        if (hide) { g->prod[0] = '\0'; snprintf(g->banner, sizeof g->banner, "220 mx%d.example.test ESMTP\r\n", (int)(rnd() % 9)); }
        else snprintf(g->banner, sizeof g->banner, "220 mx%d.example.test ESMTP %s %s ready\r\n", (int)(rnd() % 9), g->prod, v);
        break;
    case PROTO_FTP:
        if (hide) snprintf(g->banner, sizeof g->banner, "220 (%s) ready\r\n", g->prod);
        else snprintf(g->banner, sizeof g->banner, "220 (%s %s) ready\r\n", g->prod, v);
        break;
    default:
        if (hide) snprintf(g->banner, sizeof g->banner, "HTTP/1.%d 200 OK\r\nDate: Mon\r\nServer: %s\r\nContent-Length: 0\r\n\r\n", (int)(rnd() % 2), g->prod);
        else snprintf(g->banner, sizeof g->banner, "HTTP/1.%d 200 OK\r\nDate: Mon\r\n%s: %s/%s\r\nContent-Length: 0\r\n\r\n", (int)(rnd() % 2), rnd() % 2 ? "Server" : "server", g->prod, v);
        break;
    }
}

static int rc(const char *a, const char *b) {
    int x[8] = {0}, y[8] = {0}, i;
    const char *p = a; for (i = 0; i < 8 && *p; i++) { x[i] = atoi(p); while (isdigit((unsigned char)*p)) p++; if (*p == '.') p++; else break; }
    p = b; for (i = 0; i < 8 && *p; i++) { y[i] = atoi(p); while (isdigit((unsigned char)*p)) p++; if (*p == '.') p++; else break; }
    for (i = 0; i < 8; i++) if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    return 0;
}
static int sgn(int x) { return x < 0 ? -1 : x > 0; }

int main(void) {
    for (int mode = 0; mode < 2; mode++) {
        int bad = 0; char first[200] = "";
        int badp = 0;
        for (int proto = 1; proto <= 4; proto++) {
            for (int it = 0; it < 40; it++) {
                rs = 1000u * proto + it + mode * 77;
                Gen g; gen(&g, proto, mode);
                char p[64], v[64];
                int k = identify(g.banner, p, sizeof p, v, sizeof v);
                if (k != g.proto || strcmp(p, g.prod) || strcmp(v, g.ver)) { if (!bad) snprintf(first, sizeof first, "banner [%.70s] gave proto %d '%s' '%s'", g.banner, k, p, v); bad++; }
            }
        }
        (void)badp;
        CHECK(mode ? "banners that hide their version" : "banners with product and version", bad == 0, "%d of 160 wrong, e.g. %s", bad, first);
    }
    {
        const char *unk[] = { "", "hello", "SSH-3.0-x_1", "220 mail.example.test", "HTTP/2 200", "SSH-2.0-", "GET / HTTP/1.1" };
        int bad = 0;
        for (int i = 0; i < 7; i++) { char p[8] = "zz", v[8] = "zz"; int k = identify(unk[i], p, sizeof p, v, sizeof v); if (i != 5 && (k != PROTO_UNKNOWN || p[0] || v[0])) bad++; if (i == 5 && (k != PROTO_SSH || p[0])) bad++; }
        CHECK("unknown banners give empty product and version", bad == 0, "%d unknown or odd banners were misclassified", bad);
    }
    {
        // Output buffers: guard bytes around small buffers.
        int bad = 0;
        for (size_t cap = 0; cap <= 9; cap++) {
            unsigned char blk[48]; memset(blk, 0xA5, sizeof blk);
            char *p = (char *)blk + 16, *v = (char *)blk + 32;
            identify("SSH-2.0-averylongproductname_12345.6789.0 x", p, cap, v, cap);
            int ok = 1;
            for (int i = 0; i < 16; i++) if (blk[i] != 0xA5) ok = 0;
            for (int i = (int)cap; i < 16; i++) if (blk[16 + i] != 0xA5) ok = 0;
            for (int i = 32 + (int)cap; i < 48; i++) if (blk[i] != 0xA5) ok = 0;
            if (cap > 0 && (strlen(p) != (cap - 1 < 22 ? cap - 1 : 22) || strlen(v) != (cap - 1 < 12 ? cap - 1 : 12))) ok = 0;
            if (!ok) bad++;
        }
        CHECK("small output buffers are never overrun", bad == 0, "%d capacities from 0 to 9 wrote out of bounds or were not terminated", bad);
    }
    {
        int bad = 0; char first[120] = "";
        const char *vs[] = { "1.0", "1.0.0", "1.9", "1.10", "2", "2.0.1", "0.9.8", "10.1", "1.2-rc1", "3.2", "3.10.4", "0" };
        for (int i = 0; i < 12; i++) for (int j = 0; j < 12; j++) if (sgn(ver_cmp(vs[i], vs[j])) != rc(vs[i], vs[j])) { if (!bad) snprintf(first, sizeof first, "ver_cmp(\"%s\", \"%s\") = %d, expected sign %d", vs[i], vs[j], ver_cmp(vs[i], vs[j]), rc(vs[i], vs[j])); bad++; }
        CHECK("versions compare numerically", bad == 0, "%d wrong pairs, e.g. %s", bad, first);
    }
    {
        Advisory adv[] = { { "toyhttpd", "1.2" }, { "quillsh", "3.0" }, { "mailtoy", "2.5.0" }, { "toyhttpd", "0.9" } };
        struct { const char *p, *v; int want; } t[] = {
            { "toyhttpd", "1.1.9", 0 }, { "toyhttpd", "1.2", -1 }, { "toyhttpd", "1.10", -1 }, { "quillsh", "2.9", 1 }, { "quillsh", "3.0.0", -1 },
            { "mailtoy", "2.4.1", 2 }, { "mailtoy", "", -2 }, { "finchftpd", "0.1", -1 }, { "finchftpd", "", -1 }, { "toyhttpd", "", -2 }, { "mailtoy", "2.10", -1 } };
        int bad = 0; char first[120] = "";
        for (size_t i = 0; i < sizeof t / sizeof t[0]; i++) { int g = advisory_check(t[i].p, t[i].v, adv, 4); if (g != t[i].want) { if (!bad) snprintf(first, sizeof first, "%s %s: expected %d, got %d", t[i].p, t[i].v[0] ? t[i].v : "(hidden)", t[i].want, g); bad++; } }
        CHECK("advisories match older versions only", bad == 0, "%d wrong, e.g. %s", bad, first);
        Advisory one[] = { { "reedweb", "5" } };
        CHECK("a hidden version cannot be cleared", advisory_check("reedweb", "", one, 1) == -2 && advisory_check("reedweb", "4.9.9", one, 1) == 0 && advisory_check("reedweb", "5.0.0", one, 1) == -1, "hidden -> -2, 4.9.9 -> 0, 5.0.0 -> -1");
    }
    return 0;
}
