#include <stddef.h>
#include <stdint.h>
static uint8_t disk[512];
void disk_init(void) { for(size_t i=0;i<512;i++) disk[i]=0; }
int disk_read(size_t off, void *dst, size_t len) { if(off>512 || len>512-off) return -1; uint8_t *p=dst; for(size_t i=0;i<len;i++) p[i]=disk[off+i]; return 0; }
int disk_write(size_t off, const void *src, size_t len) { if(off>512 || len>512-off) return -1; const uint8_t *p=src; for(size_t i=0;i<len;i++) disk[off+i]=p[i]; return 0; }
