#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    const char *type;
    size_t offset, length;
} Carved;

/* Find complete JPEG-like and PNG-like objects in p[0..n). Return how many were stored in out. */
size_t carve(const uint8_t *p, size_t n, Carved *out, size_t max) {
    (void)p;
    (void)n;
    (void)out;
    (void)max;
    /* TODO: scan offsets; on a complete object store it and jump to its end, else advance one byte. */
    return 0;
}

int main(void) { return 0; }
