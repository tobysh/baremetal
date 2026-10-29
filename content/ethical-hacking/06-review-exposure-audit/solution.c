#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { PORT_CLOSED = 0, PORT_OPEN = 1, PORT_TIMEOUT = -1 };
enum { F_UNEXPECTED = 1, F_OUTDATED = 2, F_UNVERIFIED = 3 };
typedef struct { int (*probe)(void *ctx, int port); const char *(*banner)(void *ctx, int port); void *ctx; } HostApi;
typedef struct { const char *product; const char *fixed_in; } Advisory;
typedef struct { int port; int kind; int adv; } Finding;

int ver_cmp(const char *a, const char *b) {
    while (*a || *b) {
        long x = 0, y = 0;
        while (isdigit((unsigned char)*a)) x = x * 10 + (*a++ - '0');
        while (isdigit((unsigned char)*b)) y = y * 10 + (*b++ - '0');
        if (x != y) return x < y ? -1 : 1;
        if (*a == '.') a++;
        if (*b == '.') b++;
        if (*a && !isdigit((unsigned char)*a) && *a != '.') break;
        if (*b && !isdigit((unsigned char)*b) && *b != '.') break;
    }
    return 0;
}

static void add(Finding *out, int cap, int *n, int port, int kind, int adv) {
    if (*n < cap) { out[*n].port = port; out[*n].kind = kind; out[*n].adv = adv; (*n)++; }
}

int audit_host(const HostApi *h, int lo, int hi, const int *allowed, int na, const Advisory *adv, int nadv, Finding *out, int cap) {
    if (lo < 1 || hi > 65535 || lo > hi) return -1;
    int n = 0;
    for (int p = lo; p <= hi; p++) {
        int r = PORT_TIMEOUT;
        for (int a = 0; a < 3 && r == PORT_TIMEOUT; a++) r = h->probe(h->ctx, p);
        if (r != PORT_OPEN) continue;
        int ok = 0;
        for (int i = 0; i < na; i++) if (allowed[i] == p) ok = 1;
        if (!ok) add(out, cap, &n, p, F_UNEXPECTED, -1);
        const char *b = h->banner(h->ctx, p);
        if (!b) continue;
        const char *slash = strchr(b, '/');
        size_t pl = slash ? (size_t)(slash - b) : strlen(b);
        for (int i = 0; i < nadv; i++) {
            if (strlen(adv[i].product) != pl || strncmp(adv[i].product, b, pl)) continue;
            if (!slash) { add(out, cap, &n, p, F_UNVERIFIED, i); break; }
            if (ver_cmp(slash + 1, adv[i].fixed_in) < 0) { add(out, cap, &n, p, F_OUTDATED, i); break; }
        }
    }
    return n;
}

int main(void) {
    puts("run the tests to see your audit work");
    return 0;
}
