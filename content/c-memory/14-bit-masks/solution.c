#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t set_bit(uint32_t x, int n) {
    return x | (1u << n);
}

static uint32_t clear_bit(uint32_t x, int n) {
    return x & ~(1u << n);
}

static uint32_t toggle_bit(uint32_t x, int n) {
    return x ^ (1u << n);
}

static int test_bit(uint32_t x, int n) {
    return (x & (1u << n)) != 0;
}

int main(void) {
    uint32_t flags = 0;
    char cmd[16];
    while (scanf("%15s", cmd) == 1) {
        if (strcmp(cmd, "show") == 0) {
            printf("flags: 0x%08X (%u)\n", (unsigned)flags, (unsigned)flags);
            continue;
        }
        int n;
        if (scanf("%d", &n) != 1) break;
        if (n < 0 || n > 31) continue;
        if (strcmp(cmd, "set") == 0) flags = set_bit(flags, n);
        else if (strcmp(cmd, "clear") == 0) flags = clear_bit(flags, n);
        else if (strcmp(cmd, "toggle") == 0) flags = toggle_bit(flags, n);
        else if (strcmp(cmd, "test") == 0) printf("%d\n", test_bit(flags, n));
    }
    return 0;
}
