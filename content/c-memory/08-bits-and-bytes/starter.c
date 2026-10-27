#include <stdio.h>
#include <stdint.h>

int count_bits(uint32_t x) {
    return 0;
}

int is_power_of_two(uint32_t x) {
    return 0;
}

uint32_t swap_endian(uint32_t x) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    printf("count_bits(255) = %d (expected 8)\n", count_bits(255));
    printf("is_power_of_two(64) = %d (expected 1)\n", is_power_of_two(64));
    printf("swap_endian(0x12345678) = 0x%08X (expected 0x78563412)\n", (unsigned)swap_endian(0x12345678u));
    return 0;
}
