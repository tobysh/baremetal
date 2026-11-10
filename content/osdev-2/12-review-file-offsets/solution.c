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

static int valid_fd(int fd) {
    return fd >= 0 && fd < MAX_FDS && fds[fd].open;
}

void files_init(void) {
    for (unsigned i = 0; i < FILE_SIZE; i++)
        file_data[i] = (uint8_t)i;
    for (unsigned i = 0; i < MAX_FDS; i++)
        fds[i] = (Descriptor){ 0, 0 };
}

int file_open(void) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (!fds[i].open) {
            fds[i] = (Descriptor){ 1, 0 };   /* fresh offset for a reused slot */
            return i;
        }
    }
    return -1;
}

int file_read(int fd, void *dst, size_t n) {
    if (!valid_fd(fd))
        return -1;
    size_t left = FILE_SIZE - fds[fd].offset;   /* offset <= FILE_SIZE always */
    if (n > left)
        n = left;                               /* short read at the end */
    uint8_t *d = dst;
    for (size_t i = 0; i < n; i++)
        d[i] = file_data[fds[fd].offset + i];
    fds[fd].offset += n;
    return (int)n;
}

int file_close(int fd) {
    if (!valid_fd(fd))
        return -1;
    fds[fd].open = 0;
    return 0;
}
