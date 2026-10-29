#include <stdint.h>
#include <stdio.h>

void put_u16(uint8_t *p, uint16_t v) {
}

void put_u32(uint8_t *p, uint32_t v) {
}

uint16_t get_u16(const uint8_t *p) {
    return 0;
}

uint32_t get_u32(const uint8_t *p) {
    return 0;
}

uint32_t swap32(uint32_t v) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    uint8_t b[4];
    put_u32(b, 0x12345678u);
    printf("%02x %02x %02x %02x (expected 12 34 56 78)\n", b[0], b[1], b[2], b[3]);
    return 0;
}
