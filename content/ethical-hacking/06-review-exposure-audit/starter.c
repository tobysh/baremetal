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

// Provided: numeric dotted-version compare. Leave as is.
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

int audit_host(const HostApi *h, int lo, int hi, const int *allowed, int na, const Advisory *adv, int nadv, Finding *out, int cap) {
    (void)h; (void)lo; (void)hi; (void)allowed; (void)na; (void)adv; (void)nadv; (void)out; (void)cap;
    return 0;
}

int main(void) {
    puts("run the tests to see your audit work");
    return 0;
}
