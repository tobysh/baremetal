#include <stdio.h>
#include <stdint.h>

uint32_t set_bit(uint32_t x, int n) {
    return x | (1u << n);
}

uint32_t clear_bit(uint32_t x, int n) {
    return x & ~(1u << n);
}

uint32_t toggle_bit(uint32_t x, int n) {
    return x ^ (1u << n);
}

int test_bit(uint32_t x, int n) {
    return (x & (1u << n)) != 0;
}

int main(void) {
    printf("set_bit(0, 3) = %u\n", (unsigned)set_bit(0, 3));
    printf("clear_bit(15, 0) = %u\n", (unsigned)clear_bit(15, 0));
    printf("toggle_bit(5, 1) = %u\n", (unsigned)toggle_bit(5, 1));
    printf("test_bit(8, 3) = %d\n", test_bit(8, 3));
    return 0;
}
