#include <stddef.h>
#include <stdint.h>
static uint8_t file_data[32];
typedef struct { int open; size_t offset; } Descriptor;
static Descriptor fds[4];
void files_init(void) {}
int file_open(void) { return -1; }
int file_read(int fd, void *dst, size_t n) { return -1; }
int file_close(int fd) { return -1; }
