#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { PORT_CLOSED = 0, PORT_OPEN = 1, PORT_TIMEOUT = -1 };

int scan_ports(int (*probe)(void *ctx, int port), void *ctx, int lo, int hi, int *open, int cap, int *filtered) {
    (void)probe; (void)ctx; (void)lo; (void)hi; (void)open; (void)cap;
    *filtered = 0;
    return 0;
}

int unexpected_open(const int *open, int n, const int *allowed, int na, int *out) {
    (void)open; (void)n; (void)allowed; (void)na; (void)out;
    return 0;
}

// A tiny fake host for the Run button: ports 22 and 80 are open.
static int demo(void *ctx, int port) { (void)ctx; return port == 22 || port == 80; }
int main(void) {
    int open[16], filtered;
    int n = scan_ports(demo, NULL, 1, 100, open, 16, &filtered);
    printf("%d open, %d filtered\n", n, filtered);
    return 0;
}
