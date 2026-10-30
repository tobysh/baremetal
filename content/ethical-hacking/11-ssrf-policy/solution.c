#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { URL_OK = 0, URL_SCHEME = 1, URL_MALFORMED = 2, URL_CREDENTIALS = 3, URL_NOT_ALLOWED = 4, URL_PORT = 5, URL_INTERNAL = 6, URL_NO_ADDRESS = 7 };
typedef uint32_t (*resolve_fn)(void *ctx, const char *host, int *ok);

static int ncase_(const char *a, const char *b, size_t n) { for (size_t i = 0; i < n; i++) if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 1; return 0; }
static int match(const char *e, const char *t) {
    if (e[0] == '*' && e[1] == '.') { size_t el = strlen(e) - 1, tl = strlen(t); return tl > el && strcmp(t + tl - el, e + 1) == 0; }
    return strcmp(e, t) == 0;
}
static int internal(uint32_t ip) {
    unsigned a = ip >> 24, b = (ip >> 16) & 255;
    return a == 0 || a == 10 || a == 127 || a >= 224 || (a == 100 && b >= 64 && b <= 127) || (a == 169 && b == 254) || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168);
}

int url_policy(const char *url, resolve_fn res, void *ctx, const char *const *allow, int nallow, uint32_t *pinned) {
    const char *sep = strstr(url, "://");
    if (!sep) return URL_MALFORMED;
    size_t sl = (size_t)(sep - url);
    int https;
    if (sl == 4 && !ncase_(url, "http", 4)) https = 0;
    else if (sl == 5 && !ncase_(url, "https", 5)) https = 1;
    else return URL_SCHEME;
    const char *a = sep + 3;
    size_t al = strcspn(a, "/?#");
    if (al == 0 || al > 150) return URL_MALFORMED;
    char auth[160]; memcpy(auth, a, al); auth[al] = 0;
    int at = 0;
    for (size_t i = 0; i < al; i++) {
        unsigned char c = (unsigned char)auth[i];
        if (!(isalnum(c) || c == '.' || c == '-' || c == ':' || c == '@')) return URL_MALFORMED;
        if (c == '@') at = 1;
    }
    char *hp = strrchr(auth, '@');
    hp = hp ? hp + 1 : auth;
    char *colon = strchr(hp, ':');
    if (colon && (colon[1] == 0 || strspn(colon + 1, "0123456789") != strlen(colon + 1))) return URL_MALFORMED;
    if (colon == hp || *hp == 0) return URL_MALFORMED;
    if (at) return URL_CREDENTIALS;
    if (colon) { *colon = 0; if (strlen(colon + 1) > 5 || atoi(colon + 1) != (https ? 443 : 80)) return URL_PORT; }
    char host[160]; size_t n = strlen(auth);
    for (size_t i = 0; i < n; i++) host[i] = (char)tolower((unsigned char)auth[i]);
    host[n] = 0;
    if (n && host[n - 1] == '.') host[--n] = 0;
    if (n == 0) return URL_MALFORMED;
    int ok = 0;
    for (int i = 0; i < nallow; i++) if (match(allow[i], host)) ok = 1;
    if (!ok) return URL_NOT_ALLOWED;
    int found = 0;
    uint32_t ip = res(ctx, host, &found);
    if (!found) return URL_NO_ADDRESS;
    if (internal(ip)) return URL_INTERNAL;
    if (pinned) *pinned = ip;
    return URL_OK;
}

int redirect_policy(const char *location, resolve_fn res, void *ctx, const char *const *allow, int nallow, uint32_t *pinned) {
    if (location[0] == '/') return location[1] == '/' ? URL_MALFORMED : URL_OK;
    return url_policy(location, res, ctx, allow, nallow, pinned);
}

int main(void) {
    const char *allow[] = { "*.cdn-example.test" };
    printf("%d\n", url_policy("http://evil.test/?x=cdn-example.test", NULL, NULL, allow, 1, NULL));
    return 0;
}
