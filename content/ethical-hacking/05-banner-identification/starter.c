#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { PROTO_UNKNOWN = 0, PROTO_SSH, PROTO_SMTP, PROTO_FTP, PROTO_HTTP };
typedef struct { const char *product; const char *fixed_in; } Advisory;

int identify(const char *banner, char *product, size_t pcap, char *version, size_t vcap) {
    (void)banner;
    if (pcap) product[0] = '\0';
    if (vcap) version[0] = '\0';
    return PROTO_UNKNOWN;
}

int ver_cmp(const char *a, const char *b) {
    return strcmp(a, b);
}

int advisory_check(const char *product, const char *version, const Advisory *adv, int n) {
    (void)product; (void)version; (void)adv; (void)n;
    return -1;
}

int main(void) {
    char p[32], v[32];
    int k = identify("SSH-2.0-quillsh_3.2 hello", p, sizeof p, v, sizeof v);
    printf("%d '%s' '%s'\n", k, p, v);
    return 0;
}
