#include <stdio.h>
#include <stdint.h>

int count_bits(uint32_t x) {
    int count = 0;
    while (x != 0) {
        x &= x - 1;
        count++;
    }
    return count;
}

int is_power_of_two(uint32_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

uint32_t swap_endian(uint32_t x) {
    return ((x >> 24) & 0x000000FFu)
         | ((x >>  8) & 0x0000FF00u)
         | ((x <<  8) & 0x00FF0000u)
         | ((x << 24) & 0xFF000000u);
}

int main(void) {
    printf("count_bits(255) = %d\n", count_bits(255));
    printf("is_power_of_two(64) = %d\n", is_power_of_two(64));
    printf("swap_endian(0x12345678) = 0x%08X\n", (unsigned)swap_endian(0x12345678u));
    return 0;
}
