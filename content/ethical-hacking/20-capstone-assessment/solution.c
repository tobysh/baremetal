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

static void add(Report *r, int kind, int sev, int port, const char *title, const char *cause, const char *ev, const char *rem) {
    if (r->n >= 16) return;
    Finding *f = &r->f[r->n++];
    f->kind = kind; f->severity = sev; f->port = port;
    snprintf(f->title, sizeof f->title, "%s", title);
    snprintf(f->root_cause, sizeof f->root_cause, "%s", cause);
    snprintf(f->evidence, sizeof f->evidence, "%s", ev);
    snprintf(f->remediation, sizeof f->remediation, "%s", rem);
}

int assess(const Target *t, const Policy *p, Report *rep) {
    rep->n = 0;
    if (!p->authorized) return -2;
    if (p->scope_lo < 1 || p->scope_hi > 65535 || p->scope_lo > p->scope_hi) return -1;
    char title[160], ev[160];
    for (int port = p->scope_lo; port <= p->scope_hi; port++) {
        int r = PORT_TIMEOUT;
        for (int a = 0; a < 3 && r == PORT_TIMEOUT; a++) r = t->probe(t->ctx, port);
        if (r != PORT_OPEN) continue;
        int ok = 0;
        for (int i = 0; i < p->nallowed; i++) if (p->allowed[i] == port) ok = 1;
        if (!ok) {
            snprintf(title, sizeof title, "Unexpected service on port %d", port);
            snprintf(ev, sizeof ev, "port %d accepted a connection but is not on the approved list", port);
            add(rep, K_UNEXPECTED, 2, port, title, "A service is listening that nobody approved.", ev, "Disable the service or add it to the approved list after review.");
        }
        const char *b = t->banner(t->ctx, port);
        if (!b) continue;
        const char *slash = strchr(b, '/');
        size_t pl = slash ? (size_t)(slash - b) : strlen(b);
        for (int i = 0; i < p->nadv; i++) {
            if (strlen(p->adv[i].product) != pl || strncmp(p->adv[i].product, b, pl)) continue;
            if (!slash) {
                snprintf(title, sizeof title, "Version of service on port %d cannot be verified", port);
                snprintf(ev, sizeof ev, "port %d banner \"%.60s\" hides the version", port, b);
                add(rep, K_UNVERIFIED, 1, port, title, "The banner hides the version, so the inventory cannot show whether it is patched.", ev, "Check the installed version by hand against the advisory.");
                break;
            }
            if (ver_cmp(slash + 1, p->adv[i].fixed_in) < 0) {
                snprintf(title, sizeof title, "Outdated service on port %d", port);
                snprintf(ev, sizeof ev, "port %d banner \"%.60s\" is older than fixed version %.20s", port, b, p->adv[i].fixed_in);
                add(rep, K_OUTDATED, 3, port, title, "The service has not been updated to the fixed release.", ev, "Update the service to the fixed version or later.");
                break;
            }
        }
    }
    for (int i = 0; i < p->ndocs; i++) {
        if (p->docs[i].owner == p->tester_uid) continue;
        char out[64];
        if (t->get_doc(t->ctx, p->tester_uid, p->docs[i].id, out, sizeof out) == 200) {
            snprintf(title, sizeof title, "Documents readable by other users");
            snprintf(ev, sizeof ev, "doc %d (owner %d) returned 200 to user %d", p->docs[i].id, p->docs[i].owner, p->tester_uid);
            add(rep, K_IDOR, 4, 0, title, "The handler never compares the document owner with the caller.", ev, "Compare the owner with the session user on every request and return 404 otherwise.");
            break;
        }
    }
    for (int i = 1; i < rep->n; i++) { Finding v = rep->f[i]; int j = i; while (j && rep->f[j - 1].severity < v.severity) { rep->f[j] = rep->f[j - 1]; j--; } rep->f[j] = v; }
    return rep->n;
}

int doc_fixed(const DocStore *s, int caller, int id, char *out, size_t cap) {
    for (int i = 0; i < s->n; i++) if (s->docs[i].id == id) {
        if (s->docs[i].owner != caller) return 404;
        if (cap) snprintf(out, cap, "%s", s->docs[i].text);
        return 200;
    }
    return 404;
}

static void flat(char *dst, const char *src, size_t cap) {
    size_t i = 0;
    for (; src[i] && i + 1 < cap; i++) dst[i] = (src[i] == '\n' || src[i] == '\r') ? ' ' : src[i];
    dst[i] = '\0';
}

int report_text(const Report *rep, char *out, size_t cap) {
    static const char *SEV[5] = { "Info", "Low", "Medium", "High", "Critical" };
    static char buf[16 * 700 + 100];
    int n = snprintf(buf, sizeof buf, "REPORT: %d finding%s\n", rep->n, rep->n == 1 ? "" : "s");
    for (int i = 0; i < rep->n; i++) {
        const Finding *f = &rep->f[i];
        char a[200], b[200], c[200], d[200];
        flat(a, f->title, sizeof a); flat(b, f->root_cause, sizeof b); flat(c, f->evidence, sizeof c); flat(d, f->remediation, sizeof d);
        int sev = (f->severity >= 0 && f->severity <= 4) ? f->severity : 0;
        n += snprintf(buf + n, sizeof buf - (size_t)n, "\nFINDING: %s\nSEVERITY: %s\nROOT CAUSE: %s\nEVIDENCE:\n- %s\nREMEDIATION: %s\n", a, SEV[sev], b, c, d);
    }
    if ((size_t)n + 1 > cap) { if (cap) out[0] = '\0'; return -1; }
    memcpy(out, buf, (size_t)n + 1);
    return n;
}

int main(void) {
    puts("run the tests: they assess several simulated hosts");
    return 0;
}
