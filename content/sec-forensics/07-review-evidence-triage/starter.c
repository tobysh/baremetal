#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Write "NAME: TYPE, first bytes HEX, hash DIGEST, STATUS\n" into out (at most cap bytes). */
void report(const char *name, const uint8_t *p, size_t n, uint64_t expected, char *out, size_t cap) {
    (void)name;
    (void)p;
    (void)n;
    (void)expected;
    if (cap) out[0] = 0;
    /* TODO: signature type, 8-byte preview, FNV-1a digest, OK or CHANGED. */
}

int main(void) { return 0; }
