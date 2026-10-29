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
}

int lb_feed(LineBuf *lb, const char *data, size_t n) {
    return -1;
}

int lb_next(LineBuf *lb, char *out, size_t cap) {
    return -1;
}

int parse_command(const char *line, char verb[16], const char **arg) {
    return -1;
}

// Optional: try them with Run.
int main(void) {
    LineBuf lb;
    lb_init(&lb);
    lb_feed(&lb, "PING\r\nGET a\nPA", 14);
    char line[64];
    int n;
    while ((n = lb_next(&lb, line, sizeof line)) >= 0) printf("line (%d): %s\n", n, line);
    return 0;
}
