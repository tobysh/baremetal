#include <stddef.h>
#include <stdint.h>

#define FILE_SIZE 32
#define MAX_FDS 4

static uint8_t file_data[FILE_SIZE];

typedef struct {
    int open;        /* 1 while the slot is in use */
    size_t offset;   /* where the next read starts, 0..FILE_SIZE */
} Descriptor;

static Descriptor fds[MAX_FDS];

void files_init(void) {
    /* TODO: file_data[i] = i; close every descriptor */
}

int file_open(void) {
    /* TODO: lowest closed slot, offset 0; -1 when full */
    return -1;
}

int file_read(int fd, void *dst, size_t n) {
    /* TODO: validate fd; clamp n to the bytes left; copy; advance */
    (void)fd;
    (void)dst;
    (void)n;
    return -1;
}

int file_close(int fd) {
    /* TODO: validate fd; close it */
    (void)fd;
    return -1;
}
