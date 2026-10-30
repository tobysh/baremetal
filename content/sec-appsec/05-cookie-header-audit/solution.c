#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BAD_SECURE   1u
#define BAD_HTTPONLY 2u
#define BAD_SAMESITE 4u
#define BAD_HSTS     8u
#define BAD_NOSNIFF  16u

static const char *skip_ws(const char *p) { while (*p == ' ' || *p == '\t') p++; return p; }
static int ieq_n(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 0;
    return 1;
}
// Compare token [p, p+len) (trimmed) with a lowercase word, ignoring case.
static int tok_is(const char *p, size_t len, const char *word) {
    while (len && (*p == ' ' || *p == '\t')) { p++; len--; }
    while (len && (p[len-1] == ' ' || p[len-1] == '\t')) len--;
    return len == strlen(word) && ieq_n(p, word, len);
}

static unsigned audit_cookie(const char *v, size_t len) {
    unsigned bad = BAD_SECURE | BAD_HTTPONLY | BAD_SAMESITE;
    const char *end = v + len;
    const char *p = memchr(v, ';', len);
    while (p && p < end) {
        const char *q = p + 1;
        const char *e = memchr(q, ';', (size_t)(end - q));
        if (!e) e = end;
        const char *eq = memchr(q, '=', (size_t)(e - q));
        size_t nlen = (size_t)((eq ? eq : e) - q);
        if (tok_is(q, nlen, "secure")) bad &= ~BAD_SECURE;
        else if (tok_is(q, nlen, "httponly")) bad &= ~BAD_HTTPONLY;
        else if (tok_is(q, nlen, "samesite") && eq) {
            size_t vl = (size_t)(e - (eq + 1));
            if (tok_is(eq + 1, vl, "lax") || tok_is(eq + 1, vl, "strict") || tok_is(eq + 1, vl, "none")) bad &= ~BAD_SAMESITE;
        }
        p = e < end ? e : NULL;
    }
    return bad;
}

static int hsts_ok(const char *v, size_t len) {
    const char *end = v + len;
    const char *q = v;
    while (q < end) {
        const char *e = memchr(q, ';', (size_t)(end - q));
        if (!e) e = end;
        const char *s = skip_ws(q);
        if (e - s > 8 && ieq_n(s, "max-age=", 8)) {
            const char *d = s + 8; unsigned long long n = 0; int digits = 0;
            while (d < e && isdigit((unsigned char)*d)) { n = n * 10 + (unsigned)(*d - '0'); d++; digits++; if (n > 1000000000000ULL) break; }
            d = skip_ws(d);
            if (digits && (d >= e) && n >= 15552000ULL) return 1;
        }
        q = e + 1;
    }
    return 0;
}

unsigned audit_response(const char *response) {
    unsigned bad = 0;
    int hsts = 0, nosniff = 0;
    const char *p = response;
    // Skip the status line.
    while (*p && *p != '\n') p++;
    if (*p) p++;
    while (*p) {
        const char *eol = p; while (*eol && *eol != '\n') eol++;
        size_t len = (size_t)(eol - p);
        if (len && p[len-1] == '\r') len--;
        if (len == 0) break;  // blank line: the headers are over
        const char *colon = memchr(p, ':', len);
        if (colon) {
            size_t nlen = (size_t)(colon - p);
            const char *v = colon + 1; size_t vlen = len - nlen - 1;
            if (tok_is(p, nlen, "set-cookie")) {
                bad |= audit_cookie(v, vlen);
                // A cookie with no attributes at all lacks everything: audit_cookie starts with all bits set.
            } else if (tok_is(p, nlen, "strict-transport-security")) {
                if (hsts_ok(v, vlen)) hsts = 1;
            } else if (tok_is(p, nlen, "x-content-type-options")) {
                if (tok_is(v, vlen, "nosniff")) nosniff = 1;
            }
        }
        p = *eol ? eol + 1 : eol;
    }
    if (!hsts) bad |= BAD_HSTS;
    if (!nosniff) bad |= BAD_NOSNIFF;
    return bad;
}

int main(void) {
    const char *r = "HTTP/1.1 200 OK\r\nSet-Cookie: sid=abc; Path=/\r\n\r\nhello";
    printf("%u\n", audit_response(r));
    return 0;
}
