#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

uint16_t predict_next(uint16_t o1, uint16_t o2, uint16_t o3) {
    // find the hidden state, then step it once more
    return 0;
}

// Optional: try it with Run.
int main(void) {
    uint32_t s = 42;
    uint16_t out[4];
    for (int i = 0; i < 4; i++) { s = s * 1664525u + 1013904223u; out[i] = (uint16_t)(s >> 16); }
    printf("real: %u, predicted: %u\n", out[3], predict_next(out[0], out[1], out[2]));
    return 0;
}
