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
    uint8_t dst[6], src[6];
    uint16_t type;    /* EtherType, after unwrapping one 802.1Q tag */
    size_t payload;   /* offset of the first payload byte */
} Ethernet;

int parse_ethernet(const uint8_t *p, size_t n, Ethernet *out)
{
    if (!p || !out || n < 14)
        return -1;

    Ethernet e = {0};
    memcpy(e.dst, p, 6);
    memcpy(e.src, p + 6, 6);
    e.type = be16(p + 12);
    e.payload = 14;

    if (e.type == 0x8100) {
        if (n < 18)
            return -1;               /* the tag is not fully present */
        e.type = be16(p + 16);       /* skip the 2-byte TCI at 14..15 */
        e.payload = 18;
    }
    *out = e;
    return 0;
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

typedef struct {
    uint16_t src, dst;     /* ports */
    uint32_t seq, ack;
    uint8_t flags;         /* byte 13, unmodified */
    size_t header, payload;   /* header bytes, then n - header */
} TCP;

int parse_tcp(const uint8_t *p, size_t n, TCP *out)
{
    if (!p || !out || n < 20)
        return -1;

    /* The data offset is the high nibble, counted in 4-byte words. */
    size_t h = (size_t)(p[12] >> 4) * 4;
    if (h < 20 || h > n)
        return -1;

    *out = (TCP){be16(p), be16(p + 2), be32(p + 4), be32(p + 8), p[13], h, n - h};
    return 0;
}

typedef struct {
    uint16_t src, dst;   /* ports */
    size_t payload;      /* declared length minus the 8-byte header */
} UDP;

int parse_udp(const uint8_t *p, size_t n, UDP *out)
{
    if (!p || !out || n < 8)
        return -1;

    /* The declared length must cover the header and must not exceed the bytes we hold. */
    size_t len = be16(p + 4);
    if (len < 8 || len > n)
        return -1;

    *out = (UDP){be16(p), be16(p + 2), len - 8};
    return 0;
}

/* Label a frame by walking the layers, giving each one exactly the bytes that belong to it.
 * Return one of: invalid, arp, other, ipv4/fragment, ipv4/tcp, ipv4/udp, ipv4/udp/dns-port, ipv4/other. */
const char *classify(const uint8_t *frame, size_t n)
{
    (void)frame;
    (void)n;
    return "invalid";
}

int main(void)
{
    return 0;
}
