#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define LB_CAP 64

typedef struct {
    char buf[LB_CAP];
    size_t len;
} LineBuf;

void lb_init(LineBuf *lb) {
    lb->len = 0;
}

int lb_feed(LineBuf *lb, const char *data, size_t n) {
    if (n > LB_CAP - lb->len) return -1;
    memcpy(lb->buf + lb->len, data, n);
    lb->len += n;
    return 0;
}

int lb_next(LineBuf *lb, char *out, size_t cap) {
    char *nl = memchr(lb->buf, '\n', lb->len);
    if (nl == NULL) return -1;
    size_t pos = (size_t)(nl - lb->buf);
    size_t linelen = pos;
    if (linelen > 0 && lb->buf[linelen - 1] == '\r') linelen--;
    if (linelen >= cap) linelen = cap - 1;
    memcpy(out, lb->buf, linelen);
    out[linelen] = '\0';
    memmove(lb->buf, lb->buf + pos + 1, lb->len - pos - 1);
    lb->len -= pos + 1;
    return (int)linelen;
}

int parse_command(const char *line, char verb[16], const char **arg) {
    const char *p = line;
    while (*p == ' ') p++;
    size_t n = 0;
    while (*p && *p != ' ') {
        if (n >= 15) return -1;
        verb[n++] = (char)toupper((unsigned char)*p);
        p++;
    }
    if (n == 0) return -1;
    verb[n] = '\0';
    while (*p == ' ') p++;
    *arg = p;
    return 0;
}

int main(void) {
    LineBuf lb;
    lb_init(&lb);
    lb_feed(&lb, "PING\r\nGET a\nPA", 14);
    char line[64];
    int n;
    while ((n = lb_next(&lb, line, sizeof line)) >= 0) printf("line (%d): %s\n", n, line);
    return 0;
}
