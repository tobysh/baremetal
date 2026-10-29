#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { char title[96]; int severity; char root_cause[200]; char evidence[4][120]; int nevidence; char remediation[200]; } ReportIn;

static const char *SEV[5] = { "Info", "Low", "Medium", "High", "Critical" };
static void flat(char *dst, const char *src, size_t cap) {
    size_t i = 0;
    for (; src[i] && i + 1 < cap; i++) dst[i] = (src[i] == '\n' || src[i] == '\r') ? ' ' : src[i];
    dst[i] = '\0';
}

int render_report(const ReportIn *in, char *out, size_t cap) {
    char t[200], r[300], e[4][200], m[300];
    flat(t, in->title, sizeof t); flat(r, in->root_cause, sizeof r); flat(m, in->remediation, sizeof m);
    int ne = in->nevidence < 0 ? 0 : in->nevidence > 4 ? 4 : in->nevidence;
    for (int i = 0; i < ne; i++) flat(e[i], in->evidence[i], sizeof e[i]);
    int sev = (in->severity >= 0 && in->severity <= 4) ? in->severity : 0;
    char buf[2400];
    int n = snprintf(buf, sizeof buf, "FINDING: %s\nSEVERITY: %s\nROOT CAUSE: %s\nEVIDENCE:\n", t, SEV[sev], r);
    for (int i = 0; i < ne; i++) n += snprintf(buf + n, sizeof buf - (size_t)n, "- %s\n", e[i]);
    n += snprintf(buf + n, sizeof buf - (size_t)n, "REMEDIATION: %s\n", m);
    if ((size_t)n + 1 > cap) { if (cap) out[0] = '\0'; return -1; }
    memcpy(out, buf, (size_t)n + 1);
    return n;
}

static const char *LAB[5] = { "FINDING:", "SEVERITY:", "ROOT CAUSE:", "EVIDENCE:", "REMEDIATION:" };
static void trim(const char *s, size_t len, char *out) {
    while (len && isspace((unsigned char)*s)) { s++; len--; }
    while (len && isspace((unsigned char)s[len - 1])) len--;
    memcpy(out, s, len); out[len] = '\0';
}

int lint_report(const char *text) {
    int pos[5] = { -1, -1, -1, -1, -1 }; char val[5][400]; for (int i = 0; i < 5; i++) val[i][0] = 0;
    int bullets = 0, secret = 0, cur = -1, ln = 0;
    const char *p = text;
    while (*p) {
        const char *e = strchr(p, '\n'); size_t len = e ? (size_t)(e - p) : strlen(p);
        int lab = -1;
        for (int i = 0; i < 5; i++) if (!strncmp(p, LAB[i], strlen(LAB[i]))) lab = i;
        if (lab >= 0) {
            cur = lab;
            if (pos[lab] < 0) { pos[lab] = ln; trim(p + strlen(LAB[lab]), len - strlen(LAB[lab]), val[lab]); }
        } else if (cur == 3 && len >= 2 && !strncmp(p, "- ", 2)) {
            bullets++;
            char line[400]; size_t l = len < 399 ? len : 399; memcpy(line, p, l); line[l] = 0;
            static const char *keys[3] = { "password=", "token=", "secret=" };
            for (int k = 0; k < 3; k++) { const char *q = strstr(line, keys[k]); if (q && strncmp(q + strlen(keys[k]), "[redacted]", 10)) secret = 1; }
        }
        ln++;
        p = e ? e + 1 : p + len;
    }
    int bad = 0;
    if (pos[0] < 0 || !val[0][0]) bad |= 1;
    int sv = 0; for (int i = 0; i < 5; i++) if (!strcmp(val[1], SEV[i])) sv = 1;
    if (pos[1] < 0 || !sv) bad |= 2;
    if (pos[2] < 0 || !val[2][0]) bad |= 4;
    if (pos[3] < 0 || !bullets) bad |= 8;
    if (pos[4] < 0 || strlen(val[4]) < 15) bad |= 16;
    if (secret) bad |= 32;
    int last = -1;
    for (int i = 0; i < 5; i++) if (pos[i] >= 0) { if (pos[i] < last) bad |= 64; last = pos[i] > last ? pos[i] : last; }
    return bad;
}

int main(void) {
    ReportIn r = { "Invoice page shows other users' invoices", 3, "The handler never compares the owner with the caller.", { "GET /invoice/1043 as user 7 returned 200" }, 1, "Compare invoice.owner with the session user on every request." };
    char buf[1024];
    if (render_report(&r, buf, sizeof buf) > 0) fputs(buf, stdout);
    return 0;
}
