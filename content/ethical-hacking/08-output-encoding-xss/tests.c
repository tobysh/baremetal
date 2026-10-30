static uint32_t rs = 20261030u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }

// Reference versions that build the whole result in a big buffer.
static void r_html(const char *in, char *o) {
    o[0] = 0;
    for (; *in; in++) {
        const char *rep = NULL; char one[2] = { *in, 0 };
        if (*in == '&') rep = "&amp;"; else if (*in == '<') rep = "&lt;"; else if (*in == '>') rep = "&gt;";
        else if (*in == '"') rep = "&quot;"; else if (*in == '\'') rep = "&#39;"; else rep = one;
        strcat(o, rep);
    }
}
static void r_attr(const char *in, char *o) {
    o[0] = 0;
    for (; *in; in++) {
        unsigned char c = (unsigned char)*in; char b[8];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == ',' || c == '-' || c == '_' || c >= 0x80) { b[0] = (char)c; b[1] = 0; }
        else snprintf(b, sizeof b, "&#x%02X;", c);
        strcat(o, b);
    }
}
static int r_safe(const char *u) {
    size_t n = strlen(u);
    if (n == 0) return 0;
    for (size_t i = 0; i < n; i++) if ((unsigned char)u[i] <= 0x20 || u[i] == 0x7f || u[i] == '\\') return 0;
    if (n >= 2 && u[0] == '/' && u[1] == '/') return 0;
    size_t k = strcspn(u, ":/?#");
    if (u[k] != ':') return 1;
    char sch[12]; if (k >= sizeof sch) return 0;
    for (size_t i = 0; i < k; i++) sch[i] = (char)tolower((unsigned char)u[i]);
    sch[k] = 0;
    return !strcmp(sch, "http") || !strcmp(sch, "https") || !strcmp(sch, "mailto");
}

int main(void) {
    char big[4096], want[4096];
    // Known payloads in body context.
    {
        struct { const char *in, *out; } B[] = {
            { "plain words", "plain words" },
            { "<script>alert(1)</script>", "&lt;script&gt;alert(1)&lt;/script&gt;" },
            { "Tom & Jerry's \"show\"", "Tom &amp; Jerry&#39;s &quot;show&quot;" },
            { "&lt; already", "&amp;lt; already" },
            { "<img src=x onerror=alert(1)>", "&lt;img src=x onerror=alert(1)&gt;" },
            { "caf\xc3\xa9 \xe2\x9c\x93", "caf\xc3\xa9 \xe2\x9c\x93" },
            { "", "" },
        };
        for (size_t i = 0; i < sizeof B / sizeof *B; i++) {
            size_t n = encode_html(B[i].in, big, sizeof big);
            char nm[64]; snprintf(nm, sizeof nm, "html body example %zu", i);
            CHECK(nm, !strcmp(big, B[i].out) && n == strlen(B[i].out), "expected %s, got %s", B[i].out, big);
        }
    }
    // Attribute context.
    {
        struct { const char *in, *out; } A[] = {
            { "plainValue-1_2.3,4", "plainValue-1_2.3,4" },
            { "\" onmouseover=\"alert(1)", "&#x22;&#x20;onmouseover&#x3D;&#x22;alert&#x28;1&#x29;" },
            { "a b", "a&#x20;b" },
            { "x' onfocus='y", "x&#x27;&#x20;onfocus&#x3D;&#x27;y" },
            { "<>&`", "&#x3C;&#x3E;&#x26;&#x60;" },
            { "line\nbreak\t", "line&#x0A;break&#x09;" },
            { "caf\xc3\xa9", "caf\xc3\xa9" },
        };
        for (size_t i = 0; i < sizeof A / sizeof *A; i++) {
            size_t n = encode_attr(A[i].in, big, sizeof big);
            char nm[64]; snprintf(nm, sizeof nm, "attribute example %zu", i);
            CHECK(nm, !strcmp(big, A[i].out) && n == strlen(A[i].out), "expected %s, got %s", A[i].out, big);
        }
    }
    // Random strings, many capacities, with guard bytes.
    {
        static const char AL[] = "ab Z09<>&\"'`=/\\:;()\n\t.,-_%#?\xc3\xa9";
        int bad = 0, tot = 0, over = 0; char first[200] = "";
        for (int t = 0; t < 200; t++) {
            char in[40]; size_t len = rnd() % 30;
            for (size_t i = 0; i < len; i++) in[i] = AL[rnd() % (sizeof AL - 1)];
            in[len] = 0;
            for (int which = 0; which < 2; which++) {
                if (which == 0) r_html(in, want); else r_attr(in, want);
                size_t full = strlen(want);
                size_t caps[] = { 0, 1, 2, full, full + 1, full + 8, full ? full - 1 : 0 };
                for (int c = 0; c < 7; c++) {
                    size_t cap = caps[c];
                    unsigned char buf[300]; memset(buf, 0x5A, sizeof buf);
                    size_t got = which == 0 ? encode_html(in, (char *)buf + 8, cap) : encode_attr(in, (char *)buf + 8, cap);
                    tot++;
                    int ok = got == full;
                    if (cap) { size_t keep = cap - 1 < full ? cap - 1 : full; ok = ok && !memcmp(buf + 8, want, keep) && buf[8 + keep] == 0; }
                    for (size_t i = 0; i < 8; i++) if (buf[i] != 0x5A) over = 1;
                    size_t used = cap ? (cap - 1 < full ? cap : full + 1) : 0;
                    for (size_t i = 8 + used; i < 16 + used + 4 && i < sizeof buf; i++) if (buf[i] != 0x5A) over = 1;
                    if (!ok) { if (!bad) snprintf(first, sizeof first, "%s(\"%s\") cap %zu: returned %zu, expected %zu", which ? "encode_attr" : "encode_html", in, cap, got, full); bad++; }
                }
            }
        }
        CHECK("encoders match the rules for all inputs and capacities", bad == 0, "%d of %d wrong, e.g. %s", bad, tot, first);
        CHECK("encoders never write outside the capacity", !over, "bytes before or after the allowed area changed");
    }
    // URL policy.
    {
        const char *OKU[] = { "http://shop.test/a?b=1&c=2", "https://shop.test", "HTTPS://SHOP.TEST/x", "mailto:help@shop.test", "/orders/12", "page.html", "../up/one", "?q=1", "#top", "a/b:c", "&#x6A;avascript:alert(1)", "http:/x" };
        const char *BADU[] = { "javascript:alert(1)", "JaVaScRiPt:alert(1)", " javascript:alert(1)", "java\tscript:alert(1)", "java\nscript:alert(1)", "data:text/html,<b>x</b>", "vbscript:x", "ftp://shop.test/f", "//evil.test/x", "/\\evil.test", "\\\\evil.test", "https://shop.test/a b", "", "x:y", "file:///etc/passwd", "\x01javascript:alert(1)" };
        for (size_t i = 0; i < sizeof OKU / sizeof *OKU; i++) { char nm[60]; snprintf(nm, sizeof nm, "allowed link %zu", i); CHECK(nm, safe_url(OKU[i]) == 1, "%s should be allowed", OKU[i]); }
        for (size_t i = 0; i < sizeof BADU / sizeof *BADU; i++) { char nm[60]; snprintf(nm, sizeof nm, "refused link %zu", i); CHECK(nm, safe_url(BADU[i]) == 0, "%s should be refused", BADU[i]); }
        CHECK("NULL is refused", safe_url(NULL) == 0, "");
        static const char AL[] = "ajvsctiphmal:/\\?#. A\t0%";
        int bad = 0; char first[120] = "";
        for (int t = 0; t < 400; t++) {
            char u[24]; size_t len = 1 + rnd() % 20;
            for (size_t i = 0; i < len; i++) u[i] = AL[rnd() % (sizeof AL - 1)];
            u[len] = 0;
            if (safe_url(u) != r_safe(u)) { if (!bad) snprintf(first, sizeof first, "\"%s\": expected %d", u, r_safe(u)); bad++; }
        }
        CHECK("random URL-like strings follow the policy", bad == 0, "%d of 400 wrong, e.g. %s", bad, first);
    }
    // Composition.
    {
        char o[512], t1[512], t2[512], w[1100];
        struct { const char *text, *url; } L[] = {
            { "Docs", "https://shop.test/docs?a=1&b=2" },
            { "<b>bold</b> & more", "/orders/12" },
            { "click", "javascript:alert(1)" },
            { "x\" onmouseover=\"y", "https://a.test/\" onmouseover=\"alert(1)" },
            { "mail me", "mailto:help@shop.test" },
            { "", "#" },
        };
        int bad = 0; char first[200] = "";
        for (size_t i = 0; i < sizeof L / sizeof *L; i++) {
            if (r_safe(L[i].url)) r_attr(L[i].url, t1); else strcpy(t1, "#"); r_html(L[i].text, t2);
            snprintf(w, sizeof w, "<a href=\"%s\">%s</a>", t1, t2);
            size_t n = render_link(L[i].text, L[i].url, o, sizeof o);
            if (strcmp(o, w) || n != strlen(w)) { if (!bad) snprintf(first, sizeof first, "link %zu: expected %s, got %s", i, w, o); bad++; }
        }
        CHECK("links encode the URL as an attribute and the text as HTML", bad == 0, "%d wrong, e.g. %s", bad, first);
        render_link("Hello", "javascript:alert(1)", o, sizeof o);
        CHECK("an unsafe URL becomes a harmless #", !strcmp(o, "<a href=\"#\">Hello</a>"), "got %s", o);
        char small[16]; memset(small, 'Z', sizeof small);
        size_t n = render_link("Docs", "/a", small, 10);
        CHECK("render_link respects the capacity", n == strlen("<a href=\"&#x2F;a\">Docs</a>") && small[9] == 0 && !strncmp(small, "<a href=\"", 9) && small[10] == 'Z', "truncate to cap - 1 bytes plus a terminator and return the full length");
    }
    return 0;
}
