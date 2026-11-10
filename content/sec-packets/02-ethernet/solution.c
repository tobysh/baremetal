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

int main(void)
{
    return 0;
}
