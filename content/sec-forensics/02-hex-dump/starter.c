#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Write an xxd-style dump of p[0..n) into out. Return the length the full dump needs. */
size_t hexdump(const uint8_t *p, size_t n, char *out, size_t cap) {
    (void)p;
    (void)n;
    if (cap) out[0] = 0;
    /* TODO: one row per 16 bytes: offset, hex pairs padded to 39, two spaces, printable view. */
    return 0;
}

int main(void) { return 0; }
