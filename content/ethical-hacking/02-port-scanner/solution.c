#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { PORT_CLOSED = 0, PORT_OPEN = 1, PORT_TIMEOUT = -1 };

int scan_ports(int (*probe)(void *ctx, int port), void *ctx, int lo, int hi, int *open, int cap, int *filtered) {
    if (lo < 1 || hi > 65535 || lo > hi) return -1;
    *filtered = 0;
    int n = 0;
    for (int p = lo; p <= hi; p++) {
        int r = PORT_TIMEOUT;
        for (int a = 0; a < 3 && r == PORT_TIMEOUT; a++) r = probe(ctx, p);
        if (r == PORT_TIMEOUT) (*filtered)++;
        else if (r == PORT_OPEN && n < cap) open[n++] = p;
    }
    return n;
}

int unexpected_open(const int *open, int n, const int *allowed, int na, int *out) {
    int k = 0;
    for (int i = 0; i < n; i++) {
        int ok = 0;
        for (int j = 0; j < na; j++) if (allowed[j] == open[i]) ok = 1;
        if (!ok) out[k++] = open[i];
    }
    return k;
}

static int demo(void *ctx, int port) { (void)ctx; return port == 22 || port == 80; }
int main(void) {
    int open[16], filtered;
    int n = scan_ports(demo, NULL, 1, 100, open, 16, &filtered);
    printf("%d open, %d filtered\n", n, filtered);
    return 0;
}
