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

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static unsigned le16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }

int read_dir(const uint8_t *p, size_t n, DirEntry *out, size_t max) {
    size_t count = 0;
    for (size_t off = 0; n - off >= 32 && count < max; off += 32) {
        const uint8_t *e = p + off;
        if (e[0] == 0) break;
        if (e[11] == 0x0f || (e[11] & 0x08)) continue;

        DirEntry v = {0};
        v.deleted = e[0] == 0xe5;
        v.is_dir = (e[11] & 0x10) != 0;

        size_t base = 8, ext = 3;
        while (base && e[base - 1] == ' ') base--;
        while (ext && e[8 + ext - 1] == ' ') ext--;

        size_t k = 0;
        for (size_t j = 0; j < base; j++)
            v.name[k++] = (j == 0 && v.deleted) ? '_' : (char)e[j];
        if (ext) v.name[k++] = '.';
        for (size_t j = 0; j < ext; j++) v.name[k++] = (char)e[8 + j];
        v.name[k] = 0;

        v.cluster = (uint32_t)le16(e + 20) << 16 | le16(e + 26);
        v.size = le32(e + 28);
        out[count++] = v;
    }
    return (int)count;
}

int main(void) { return 0; }
