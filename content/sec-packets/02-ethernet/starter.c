#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    uint8_t dst[6], src[6];
    uint16_t type;    /* EtherType, after unwrapping one 802.1Q tag */
    size_t payload;   /* offset of the first payload byte */
} Ethernet;

/* Decode an Ethernet II header with at most one VLAN tag. Return 0 or -1. */
int parse_ethernet(const uint8_t *p, size_t n, Ethernet *out)
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
