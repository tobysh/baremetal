#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_FRAME 1024

static inline void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static inline uint16_t get_u16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

// Reads until n bytes arrive; returns fewer only at end of file, -1 on error.
static inline ssize_t read_full(int fd, void *buf, size_t n) {
    size_t done = 0;
    while (done < n) {
        ssize_t r = read(fd, (char *)buf + done, n - done);
        if (r == 0) break;
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        done += (size_t)r;
    }
    return (ssize_t)done;
}

static inline ssize_t write_full(int fd, const void *buf, size_t n) {
    size_t done = 0;
    while (done < n) {
        ssize_t w = write(fd, (const char *)buf + done, n - done);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        done += (size_t)w;
    }
    return (ssize_t)done;
}

int send_frame(int fd, const uint8_t *payload, size_t n) {
    return -1;
}

int recv_frame(int fd, uint8_t *buf, size_t cap) {
    return -1;
}

int serve_reversed(int fd) {
    return -1;
}

// Optional: try them with Run.
int main(void) {
    int p[2];
    if (pipe(p) != 0) return 1;
    send_frame(p[1], (const uint8_t *)"hello", 5);
    uint8_t buf[64];
    int n = recv_frame(p[0], buf, sizeof buf);
    printf("received %d bytes (expected 5)\n", n);
    return 0;
}
