#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

int crack_xor(const uint8_t *cipher, size_t n) {
    // try all 256 keys, score each result, return the best key
    return 0;
}

// Optional: try it with Run.
int main(void) {
    const char *msg = "this is a short test";
    uint8_t c[32];
    for (size_t i = 0; msg[i]; i++) c[i] = (uint8_t)(msg[i] ^ 0x5a);
    printf("guess: %d\n", crack_xor(c, strlen(msg)));
    return 0;
}
