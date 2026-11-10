#include <stddef.h>
#include <stdint.h>

#define DISK_SIZE 512

static uint8_t disk[DISK_SIZE];

void disk_init(void) {
    /* TODO: a fresh disk reads as zeros */
}

int disk_read(size_t offset, void *dst, size_t len) {
    /* TODO: validate [offset, offset + len) without overflow, then copy */
    (void)offset;
    (void)dst;
    (void)len;
    return -1;
}

int disk_write(size_t offset, const void *src, size_t len) {
    /* TODO: same check, then copy the other way */
    (void)offset;
    (void)src;
    (void)len;
    return -1;
}
