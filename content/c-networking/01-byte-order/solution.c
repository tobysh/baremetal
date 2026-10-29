#include <stdint.h>
#include <stdio.h>

void put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

uint16_t get_u16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

uint32_t get_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

uint32_t swap32(uint32_t v) {
    return (v >> 24) | ((v >> 8) & 0xff00u) | ((v << 8) & 0xff0000u) | (v << 24);
}

int main(void) {
    uint8_t b[4];
    put_u32(b, 0x12345678u);
    printf("%02x %02x %02x %02x (expected 12 34 56 78)\n", b[0], b[1], b[2], b[3]);
    return 0;
}
