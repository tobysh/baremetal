#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    uint32_t src, dst;   /* host-order numbers: 192.0.2.10 is 0xc000020a */
    uint16_t total;      /* total length claimed by the header */
    uint8_t protocol;
    size_t header;       /* header length in bytes (IHL * 4) */
    int fragmented;      /* 1 if MF is set or the fragment offset is nonzero */
} IPv4;

/* Decode an IPv4 header and check its length claims against n. Return 0 or -1. */
int parse_ipv4(const uint8_t *p, size_t n, IPv4 *out)
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
