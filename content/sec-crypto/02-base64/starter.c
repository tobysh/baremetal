#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

size_t b64_encode(const uint8_t *in, size_t n, char *out) {
    // write the base64 text, a '\0', and return its length
    return 0;
}

// Optional: try it with Run.
int main(void) {
    char text[16];
    b64_encode((const uint8_t *)"Man", 3, text);
    printf("%s\n", text);
    return 0;
}
