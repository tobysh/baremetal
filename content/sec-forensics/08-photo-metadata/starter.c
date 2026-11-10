#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    char make[64], model[64];
    uint32_t width, height;
    char datetime[32];
} Meta;

/* Decode the TIFF-style directory in p[0..n). Return 0 on success, -1 on malformed data. */
int read_meta(const uint8_t *p, size_t n, Meta *out) {
    (void)p;
    (void)n;
    (void)out;
    /* TODO: byte order, 42, directory offset and count, then each supported tag. */
    return -1;
}

/* Convert "YYYY:MM:DD HH:MM:SS" (UTC) to Unix seconds, or -1 if invalid. */
long long to_unix(const char *s) {
    (void)s;
    return -1;
}

int main(void) { return 0; }
