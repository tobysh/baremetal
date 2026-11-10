#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    uint16_t src, dst;     /* ports */
    uint32_t seq, ack;
    uint8_t flags;         /* byte 13, unmodified */
    size_t header, payload;   /* header bytes, then n - header */
} TCP;

/* Decode a TCP header from the span `p`, `n` bytes long. Return 0 or -1. */
int parse_tcp(const uint8_t *p, size_t n, TCP *out)
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
