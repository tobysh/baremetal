#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    const char *name;
    const uint8_t *data;
    size_t n;
} Item;

/* 64-bit FNV-1a: h = 0xcbf29ce484222325; for each byte h ^= b; h *= 0x100000001b3. */
uint64_t fnv1a64(const uint8_t *p, size_t n) {
    (void)p;
    (void)n;
    return 0;
}

/* status[i]: 0 digest matches, 1 name listed but digest differs, 2 name not in the manifest. */
int audit(const char *manifest, const Item *items, int n, int *status) {
    (void)manifest;
    (void)items;
    for (int i = 0; i < n; i++) status[i] = 2;
    return n;
}

int main(void) { return 0; }
