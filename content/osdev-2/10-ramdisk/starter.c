#include <stddef.h>
#include <stdint.h>
static uint8_t disk[512];
void disk_init(void) {}
int disk_read(size_t off, void *dst, size_t len) { return -1; }
int disk_write(size_t off, const void *src, size_t len) { return -1; }
