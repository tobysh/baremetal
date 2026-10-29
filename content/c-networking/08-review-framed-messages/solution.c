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
    if (n > MAX_FRAME) return -1;
    uint8_t out[2 + MAX_FRAME];
    put_u16(out, (uint16_t)n);
    memcpy(out + 2, payload, n);
    return write_full(fd, out, 2 + n) < 0 ? -1 : 0;
}

int recv_frame(int fd, uint8_t *buf, size_t cap) {
    uint8_t hdr[2];
    ssize_t r = read_full(fd, hdr, 2);
    if (r == 0) return -2;
    if (r != 2) return -1;
    size_t len = get_u16(hdr);
    if (len > MAX_FRAME || len > cap) return -1;
    if (len == 0) return 0;
    r = read_full(fd, buf, len);
    if (r != (ssize_t)len) return -1;
    return (int)len;
}

int serve_reversed(int fd) {
    uint8_t buf[MAX_FRAME];
    int frames = 0;
    for (;;) {
        int n = recv_frame(fd, buf, sizeof buf);
        if (n == -2) return frames;
        if (n < 0) return -1;
        for (int i = 0, j = n - 1; i < j; i++, j--) {
            uint8_t t = buf[i];
            buf[i] = buf[j];
            buf[j] = t;
        }
        if (send_frame(fd, buf, (size_t)n) < 0) return -1;
        frames++;
    }
}

int main(void) {
    int p[2];
    if (pipe(p) != 0) return 1;
    send_frame(p[1], (const uint8_t *)"hello", 5);
    uint8_t buf[64];
    int n = recv_frame(p[0], buf, sizeof buf);
    printf("received %d bytes (expected 5)\n", n);
    return 0;
}
