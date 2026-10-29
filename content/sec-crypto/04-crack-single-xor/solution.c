#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static long score(const uint8_t *c, size_t n, uint8_t k) {
    long s = 0;
    for (size_t i = 0; i < n; i++) {
        uint8_t p = c[i] ^ k;
        if (p == ' ') s += 12;
        else if (p == '\n') s += 1;
        else if (p < 32 || p > 126) s -= 50;
        else if ((p | 32) >= 'a' && (p | 32) <= 'z') {
            const char *order = "etaoinshrdlcumwfgypbvkjxqz";
            const char *at = strchr(order, p | 32);
            s += 10 - (at - order) / 3;
        } else if (strchr(".,'-;:!?", p)) s += 2;
        else if (p >= '0' && p <= '9') s += 1;
    }
    return s;
}

int crack_xor(const uint8_t *cipher, size_t n) {
    int best = 0;
    long best_score = -1000000000L;
    for (int k = 0; k < 256; k++) {
        long s = score(cipher, n, (uint8_t)k);
        if (s > best_score) { best_score = s; best = k; }
    }
    return best;
}

int main(void) { return 0; }
