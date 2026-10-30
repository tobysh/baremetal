#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { PORT_CLOSED = 0, PORT_OPEN = 1, PORT_TIMEOUT = -1 };
enum { K_UNEXPECTED = 1, K_OUTDATED = 2, K_UNVERIFIED = 3, K_IDOR = 4 };
typedef struct { const char *product; const char *fixed_in; } Advisory;
typedef struct {
    int (*probe)(void *ctx, int port);
    const char *(*banner)(void *ctx, int port);
    int (*get_doc)(void *ctx, int caller_uid, int doc_id, char *out, size_t cap);
    void *ctx;
} Target;
typedef struct {
    int authorized;
    int scope_lo, scope_hi;
    int allowed[8]; int nallowed;
    Advisory adv[4]; int nadv;
    int tester_uid;
    struct { int id, owner; } docs[8]; int ndocs;
} Policy;
typedef struct { int kind, severity, port; char title[80], root_cause[120], evidence[120], remediation[120]; } Finding;
typedef struct { Finding f[16]; int n; } Report;
typedef struct { int id, owner; char text[32]; } Doc;
typedef struct { Doc docs[8]; int n; } DocStore;

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

int assess(const Target *t, const Policy *p, Report *rep) {
    (void)t; (void)p;
    rep->n = 0;
    return 0;
}

// The repaired handler. Fix it: only the owner may read a document.
int doc_fixed(const DocStore *s, int caller, int id, char *out, size_t cap) {
    for (int i = 0; i < s->n; i++) if (s->docs[i].id == id) {
        (void)caller;
        if (cap) snprintf(out, cap, "%s", s->docs[i].text);
        return 200;
    }
    return 404;
}

int report_text(const Report *rep, char *out, size_t cap) {
    (void)rep; (void)out; (void)cap;
    return -1;
}

int main(void) {
    puts("run the tests: they assess several simulated hosts");
    return 0;
}
