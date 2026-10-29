#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

size_t b64_encode(const uint8_t *in, size_t n, char *out) {
    static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    for (size_t i = 0; i < n; i += 3) {
        size_t left = n - i;
        uint32_t v = (uint32_t)in[i] << 16;
        if (left > 1) v |= (uint32_t)in[i + 1] << 8;
        if (left > 2) v |= in[i + 2];
        out[o++] = T[(v >> 18) & 63];
        out[o++] = T[(v >> 12) & 63];
        out[o++] = left > 1 ? T[(v >> 6) & 63] : '=';
        out[o++] = left > 2 ? T[v & 63] : '=';
    }
    out[o] = '\0';
    return o;
}

int main(void) { return 0; }
