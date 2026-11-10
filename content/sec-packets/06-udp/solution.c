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

int main(void)
{
    return 0;
}
