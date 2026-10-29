#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { PROTO_UNKNOWN = 0, PROTO_SSH, PROTO_SMTP, PROTO_FTP, PROTO_HTTP };
typedef struct { const char *product; const char *fixed_in; } Advisory;

static void put(char *dst, size_t cap, const char *s, size_t len) {
    if (!cap) return;
    if (len > cap - 1) len = cap - 1;
    memcpy(dst, s, len);
    dst[len] = '\0';
}
static size_t word(const char *s) { size_t n = 0; while (s[n] && s[n] != ' ' && s[n] != '\r' && s[n] != '\n') n++; return n; }

int identify(const char *b, char *product, size_t pcap, char *version, size_t vcap) {
    const char *ps = "", *vs = "";
    size_t pl = 0, vl = 0;
    int proto = PROTO_UNKNOWN;
    if (!strncmp(b, "SSH-2.0-", 8) || !strncmp(b, "SSH-1.99-", 9)) {
        proto = PROTO_SSH;
        const char *t = b + (b[4] == '2' ? 8 : 9);
        size_t n = word(t), cut = n;
        for (size_t i = 0; i < n; i++) if (t[i] == '_') cut = i;
        ps = t; pl = cut;
        if (cut < n) { vs = t + cut + 1; vl = n - cut - 1; }
    } else if (!strncmp(b, "220 (", 5)) {
        proto = PROTO_FTP;
        const char *t = b + 5;
        size_t i = 0; while (t[i] && t[i] != ' ' && t[i] != ')') i++;
        ps = t; pl = i;
        if (t[i] == ' ') { vs = t + i + 1; size_t j = 0; while (vs[j] && vs[j] != ')') j++; vl = j; }
    } else if (!strncmp(b, "220 ", 4)) {
        const char *e = strstr(b, " ESMTP");
        if (e && (e[6] == '\0' || e[6] == ' ' || e[6] == '\r' || e[6] == '\n')) {
            proto = PROTO_SMTP;
            const char *t = e + 6;
            if (*t == ' ') {
                t++;
                ps = t; pl = word(t);
                t += pl;
                if (*t == ' ' && isdigit((unsigned char)t[1])) { vs = t + 1; vl = word(vs); }
            }
        }
    } else if (!strncmp(b, "HTTP/1.", 7)) {
        proto = PROTO_HTTP;
        const char *l = b;
        while (l && *l) {
            if (tolower((unsigned char)l[0]) == 's' && tolower((unsigned char)l[1]) == 'e' && tolower((unsigned char)l[2]) == 'r' && tolower((unsigned char)l[3]) == 'v' && tolower((unsigned char)l[4]) == 'e' && tolower((unsigned char)l[5]) == 'r' && l[6] == ':') {
                const char *t = l + 7; while (*t == ' ') t++;
                size_t n = 0; while (t[n] && t[n] != '\r' && t[n] != '\n') n++;
                size_t cut = n; for (size_t i = 0; i < n; i++) if (t[i] == '/') { cut = i; break; }
                ps = t; pl = cut;
                if (cut < n) { vs = t + cut + 1; vl = n - cut - 1; }
                break;
            }
            l = strchr(l, '\n'); if (l) l++;
        }
    }
    put(product, pcap, ps, pl);
    put(version, vcap, vs, vl);
    return proto;
}

int ver_cmp(const char *a, const char *b) {
    while (*a || *b) {
        long x = 0, y = 0;
        while (isdigit((unsigned char)*a)) x = x * 10 + (*a++ - '0');
        while (isdigit((unsigned char)*b)) y = y * 10 + (*b++ - '0');
        if (x != y) return x < y ? -1 : 1;
        if (*a == '.') a++; else if (*a) break;
        if (*b == '.') b++; else if (*b) break;
        if ((!*a || !isdigit((unsigned char)*a)) && (!*b || !isdigit((unsigned char)*b)) && (*a != 0 || *b != 0)) break;
    }
    return 0;
}

int advisory_check(const char *product, const char *version, const Advisory *adv, int n) {
    for (int i = 0; i < n; i++) {
        if (strcmp(adv[i].product, product)) continue;
        if (!version[0]) return -2;
        if (ver_cmp(version, adv[i].fixed_in) < 0) return i;
    }
    return -1;
}

int main(void) {
    char p[32], v[32];
    int k = identify("SSH-2.0-quillsh_3.2 hello", p, sizeof p, v, sizeof v);
    printf("%d '%s' '%s'\n", k, p, v);
    return 0;
}
