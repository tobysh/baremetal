#include <stddef.h>
#include <stdint.h>

#define DISK_SIZE 512

static uint8_t disk[DISK_SIZE];

/* True when [offset, offset + len) lies inside the disk. Never adds. */
static int in_range(size_t offset, size_t len) {
    if (offset > DISK_SIZE)
        return 0;
    return len <= DISK_SIZE - offset;   /* cannot underflow: offset <= 512 */
}

void disk_init(void) {
    for (size_t i = 0; i < DISK_SIZE; i++)
        disk[i] = 0;
}

int disk_read(size_t offset, void *dst, size_t len) {
    if (!in_range(offset, len))
        return -1;                      /* checked before any byte moves */
    uint8_t *d = dst;
    for (size_t i = 0; i < len; i++)
        d[i] = disk[offset + i];
    return 0;
}

int disk_write(size_t offset, const void *src, size_t len) {
    if (!in_range(offset, len))
        return -1;
    const uint8_t *s = src;
    for (size_t i = 0; i < len; i++)
        disk[offset + i] = s[i];
    return 0;
}
