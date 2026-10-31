#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

uint32_t mix(const unsigned char *p, size_t n) {
    uint32_t h = 0x1F3D5B79u;
    for (size_t i = 0; i < n; i++) {
        h = ((h << 5) | (h >> 27)) ^ p[i];
        h *= 0x2545F491u;
    }
    return h ^ (h >> 15);
}

int main(void) {
    printf("%u\n", (unsigned)mix((const unsigned char *)"abc", 3));
    return 0;
}
