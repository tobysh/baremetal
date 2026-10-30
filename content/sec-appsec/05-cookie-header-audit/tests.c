static uint32_t rs = 987654321u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static const char *pick(const char *const *a, int n) { return a[rnd() % (uint32_t)n]; }
static void lower_some(char *s) { if (rnd() & 1) for (; *s; s++) *s = (char)tolower((unsigned char)*s); else if (rnd() & 1) for (; *s; s++) *s = (char)toupper((unsigned char)*s); }

int main(void) {
    // Fixed, readable responses.
    struct { const char *name; const char *resp; unsigned want; } F[] = {
        { "a fully hardened response",
          "HTTP/1.1 200 OK\r\nSet-Cookie: sid=1; Path=/; Secure; HttpOnly; SameSite=Lax\r\nStrict-Transport-Security: max-age=31536000; includeSubDomains\r\nX-Content-Type-Options: nosniff\r\n\r\nbody", 0 },
        { "a bare cookie and no headers",
          "HTTP/1.1 200 OK\r\nSet-Cookie: sid=1\r\n\r\n", 1 | 2 | 4 | 8 | 16 },
        { "no cookies is not a cookie problem",
          "HTTP/1.1 200 OK\nStrict-Transport-Security: max-age=63072000\nX-Content-Type-Options: nosniff\n\n", 0 },
        { "attribute words in the cookie name do not count",
          "HTTP/1.1 200 OK\r\nSet-Cookie: Secure=1; Path=/\r\nStrict-Transport-Security: max-age=31536000\r\nX-Content-Type-Options: nosniff\r\n\r\n", 1 | 2 | 4 },
        { "a cookie in the body is ignored",
          "HTTP/1.1 200 OK\r\nStrict-Transport-Security: max-age=31536000\r\nX-Content-Type-Options: nosniff\r\n\r\nSet-Cookie: x=1\r\nStrict-Transport-Security: max-age=1\r\n", 0 },
        { "one weak cookie among good ones",
          "HTTP/1.1 200 OK\r\nSet-Cookie: a=1; Secure; HttpOnly; SameSite=Strict\r\nSet-Cookie: b=2; Secure; SameSite=None\r\nSet-Cookie: c=3; Secure; HttpOnly; SameSite=Lax\r\nStrict-Transport-Security: max-age=15552000\r\nX-Content-Type-Options: nosniff\r\n\r\n", 2 },
        { "HSTS one second under the limit",
          "HTTP/1.1 200 OK\r\nStrict-Transport-Security: max-age=15551999\r\nX-Content-Type-Options: nosniff\r\n\r\n", 8 },
        { "an unknown SameSite value is as good as none",
          "HTTP/1.1 200 OK\r\nSet-Cookie: a=1; Secure; HttpOnly; SameSite=Weak\r\nStrict-Transport-Security: max-age=31536000\r\nX-Content-Type-Options: nosniff\r\n\r\n", 4 },
        { "a look-alike header name is not HSTS",
          "HTTP/1.1 200 OK\r\nX-Strict-Transport-Security: max-age=31536000\r\nX-Content-Type-Options: sniff\r\n\r\n", 8 | 16 },
    };
    for (size_t i = 0; i < sizeof F / sizeof *F; i++) {
        unsigned got = audit_response(F[i].resp);
        CHECK(F[i].name, got == F[i].want, "expected flags %u, got %u", F[i].want, got);
    }
    // Generated responses: the expected flags follow from the choices made.
    static const char *EOLS[] = { "\r\n", "\n" };
    static const char *HSTS_GOOD[] = { "max-age=31536000", "max-age=15552000", "max-age=63072000; includeSubDomains; preload", "includeSubDomains; max-age=20000000", "  max-age=31536000  " };
    static const char *HSTS_BAD[] = { "max-age=0", "max-age=300", "max-age=15551999", "includeSubDomains", "max-age=abc", "max-age=" };
    int bad = 0, tot = 0; char first[200] = "";
    for (int t = 0; t < 400; t++) {
        char resp[1800]; size_t k = 0; const char *eol = EOLS[rnd() & 1];
        unsigned want = 0;
        k += (size_t)snprintf(resp + k, sizeof resp - k, "HTTP/1.1 200 OK%sServer: toy%sContent-Type: text/html%s", eol, eol, eol);
        int ncook = (int)(rnd() % 4);
        for (int c = 0; c < ncook; c++) {
            char name[20], attrs[160] = ""; int sec = rnd() % 4 != 0, ho = rnd() % 4 != 0, ss = (int)(rnd() % 7);
            static const char *NAMES[] = { "sid", "Secure", "httponly", "theme", "SameSite" };
            snprintf(name, sizeof name, "%s=%s", pick(NAMES, 5), rnd() & 1 ? "v1" : "Lax");
            strcat(attrs, "; Path=/");
            char a[24];
            if (sec) { snprintf(a, sizeof a, "; %s", pick((const char *const[]){ "Secure", "secure", "SECURE" }, 3)); strcat(attrs, a); }
            if (ho) { snprintf(a, sizeof a, ";%s", pick((const char *const[]){ "HttpOnly", "httponly", " HTTPONLY" }, 3)); strcat(attrs, a); }
            int ssok = 0;
            switch (ss) {
            case 0: break;
            case 1: strcat(attrs, "; SameSite=Lax"); ssok = 1; break;
            case 2: strcat(attrs, "; samesite=strict"); ssok = 1; break;
            case 3: strcat(attrs, "; SameSite=None"); ssok = 1; break;
            case 4: strcat(attrs, "; SameSite=Weak"); break;
            case 5: strcat(attrs, "; SameSite"); break;
            default: strcat(attrs, " ; SameSite = Strict ;"); ssok = 1; break;
            }
            if (!sec) want |= 1; if (!ho) want |= 2; if (!ssok) want |= 4;
            char hn[16] = "Set-Cookie"; lower_some(hn);
            k += (size_t)snprintf(resp + k, sizeof resp - k, "%s: %s%s%s", hn, name, attrs, eol);
        }
        int h = (int)(rnd() % 4);
        if (h == 0) want |= 8;
        else if (h == 1) { k += (size_t)snprintf(resp + k, sizeof resp - k, "Strict-Transport-Security: %s%s", pick(HSTS_GOOD, 5), eol); }
        else if (h == 2) { k += (size_t)snprintf(resp + k, sizeof resp - k, "strict-transport-security: %s%s", pick(HSTS_BAD, 6), eol); want |= 8; }
        else { k += (size_t)snprintf(resp + k, sizeof resp - k, "STRICT-TRANSPORT-SECURITY: %s%s", pick(HSTS_GOOD, 5), eol); }
        int n = (int)(rnd() % 3);
        if (n == 0) want |= 16;
        else if (n == 1) k += (size_t)snprintf(resp + k, sizeof resp - k, "X-Content-Type-Options: nosniff%s", eol);
        else { k += (size_t)snprintf(resp + k, sizeof resp - k, "X-Content-Type-Options: sniff%s", eol); want |= 16; }
        k += (size_t)snprintf(resp + k, sizeof resp - k, "%sSet-Cookie: decoy=1%sStrict-Transport-Security: max-age=99999999%sX-Content-Type-Options: nosniff%s", eol, eol, eol, eol);
        unsigned got = audit_response(resp);
        tot++;
        if (got != want) { if (!bad) snprintf(first, sizeof first, "trial %d: expected %u, got %u", t, want, got); bad++; }
    }
    CHECK("generated responses are audited correctly", bad == 0, "%d of %d wrong, e.g. %s", bad, tot, first);
    return 0;
}
