#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

void hex_encode(const uint8_t *in, size_t n, char *out) {
    // write 2 * n lowercase hex digits, then '\0'
}

int hex_decode(const char *hex, uint8_t *out) {
    // return the number of bytes written, or -1 for bad input
    return 0;
}

// Optional: try them with Run.
int main(void) {
    uint8_t data[] = {0xde, 0xad, 0xbe, 0xef};
    char text[9];
    hex_encode(data, 4, text);
    printf("%s\n", text);
    return 0;
}
