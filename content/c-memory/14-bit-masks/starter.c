#include <stdio.h>
#include <stdint.h>

uint32_t set_bit(uint32_t x, int n) {
    return x;
}

uint32_t clear_bit(uint32_t x, int n) {
    return x;
}

uint32_t toggle_bit(uint32_t x, int n) {
    return x;
}

int test_bit(uint32_t x, int n) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    printf("set_bit(0, 3) = %u (expected 8)\n", (unsigned)set_bit(0, 3));
    printf("clear_bit(15, 0) = %u (expected 14)\n", (unsigned)clear_bit(15, 0));
    printf("toggle_bit(5, 1) = %u (expected 7)\n", (unsigned)toggle_bit(5, 1));
    printf("test_bit(8, 3) = %d (expected 1)\n", test_bit(8, 3));
    return 0;
}
