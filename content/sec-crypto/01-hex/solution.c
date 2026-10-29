#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void hex_encode(const uint8_t *in, size_t n, char *out) {
    static const char D[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i] = D[in[i] >> 4];
        out[2 * i + 1] = D[in[i] & 15];
    }
    out[2 * n] = '\0';
}

int hex_decode(const char *hex, uint8_t *out) {
    size_t len = strlen(hex);
    if (len % 2) return -1;
    for (size_t i = 0; i < len / 2; i++) {
        int hi = nibble(hex[2 * i]), lo = nibble(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return (int)(len / 2);
}

int main(void) { return 0; }
