#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef struct {
    char name[13];
    int deleted, is_dir;
    uint32_t size, cluster;
} DirEntry;

/* Decode 32-byte FAT short-name records. Return the number stored in out (at most max). */
int read_dir(const uint8_t *p, size_t n, DirEntry *out, size_t max) {
    (void)p;
    (void)n;
    (void)out;
    (void)max;
    /* TODO: per record: stop on 0x00, skip LFN and labels, keep 0xe5 as deleted with '_'. */
    return 0;
}

int main(void) { return 0; }
