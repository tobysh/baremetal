#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static uint32_t step(uint32_t s) { return s * 1664525u + 1013904223u; }

uint16_t predict_next(uint16_t o1, uint16_t o2, uint16_t o3) {
    for (uint32_t low = 0; low < 65536; low++) {
        uint32_t s = ((uint32_t)o1 << 16) | low;
        uint32_t s2 = step(s), s3 = step(s2);
        if ((s2 >> 16) == o2 && (s3 >> 16) == o3) return (uint16_t)(step(s3) >> 16);
    }
    return 0;
}

int main(void) { return 0; }
