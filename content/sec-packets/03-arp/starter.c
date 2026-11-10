#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    uint16_t opcode;       /* 1 request, 2 reply */
    uint32_t sender, target;   /* IPv4 addresses as host-order numbers */
} ARP;

/* Decode an Ethernet/IPv4 ARP message (28 bytes). Return 0 or -1. */
int parse_arp(const uint8_t *p, size_t n, ARP *out)
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
