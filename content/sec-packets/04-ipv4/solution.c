#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((unsigned)p[0] << 8 | p[1]);
}

static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

typedef struct {
    uint32_t src, dst;   /* host-order numbers: 192.0.2.10 is 0xc000020a */
    uint16_t total;      /* total length claimed by the header */
    uint8_t protocol;
    size_t header;       /* header length in bytes (IHL * 4) */
    int fragmented;      /* 1 if MF is set or the fragment offset is nonzero */
} IPv4;

int parse_ipv4(const uint8_t *p, size_t n, IPv4 *out)
{
    if (!p || !out || n < 20 || p[0] >> 4 != 4)
        return -1;

    size_t h = (size_t)(p[0] & 15) * 4;
    uint16_t total = be16(p + 2);
    /* Header inside the span, total at least the header, total inside the span. */
    if (h < 20 || h > n || total < h || total > n)
        return -1;

    /* 0x3fff keeps More Fragments and the offset; Don't Fragment and the reserved bit are ignored. */
    int fragmented = (be16(p + 6) & 0x3fff) != 0;
    *out = (IPv4){be32(p + 12), be32(p + 16), total, p[9], h, fragmented};
    return 0;
}

int main(void)
{
    return 0;
}
