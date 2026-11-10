#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    int bootable;
    uint8_t type;
    uint32_t start_lba, sectors;
} Partition;

/* Decode the partition slots of a 512-byte MBR. Return -1 when the signature is wrong. */
int parse_mbr(const uint8_t *p, Partition out[4]) {
    (void)p;
    (void)out;
    /* TODO: check 55 aa, walk the four slots at 446, keep the non-empty ones. */
    return -1;
}

/* Name a partition type byte; unlisted values are "Unknown". */
const char *type_name(uint8_t t) {
    (void)t;
    return "Unknown";
}

int main(void) { return 0; }
