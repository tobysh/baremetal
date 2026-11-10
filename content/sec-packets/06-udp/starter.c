#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    uint16_t src, dst;   /* ports */
    size_t payload;      /* declared length minus the 8-byte header */
} UDP;

/* Decode a UDP header and the payload size from its declared length. Return 0 or -1. */
int parse_udp(const uint8_t *p, size_t n, UDP *out)
{
    (void)p;
    (void)n;
    (void)out;
    return -1;
}

int main(void)
{
    return 0;
}
