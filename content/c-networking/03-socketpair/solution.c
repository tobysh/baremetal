#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

static inline void put_u32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static inline uint32_t get_u32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

int make_channel(int fds[2]) {
    return socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0 ? 0 : -1;
}

int server_step(int fd) {
    uint8_t buf[4];
    ssize_t n = read(fd, buf, 4);
    if (n == 0) return 0;
    if (n != 4) return -1;
    put_u32(buf, get_u32(buf) * 2);
    if (write(fd, buf, 4) != 4) return -1;
    return 1;
}

int client_call(int fd, uint32_t x, uint32_t *result) {
    uint8_t buf[4];
    put_u32(buf, x);
    if (write(fd, buf, 4) != 4) return -1;
    if (read(fd, buf, 4) != 4) return -1;
    *result = get_u32(buf);
    return 0;
}

int main(void) {
    int fds[2];
    if (make_channel(fds) != 0) { printf("make_channel failed\n"); return 1; }
    uint8_t req[4];
    put_u32(req, 21);
    write(fds[0], req, 4);
    printf("served: %d\n", server_step(fds[1]));
    uint8_t rep[4];
    read(fds[0], rep, 4);
    printf("reply = %u (expected 42)\n", get_u32(rep));
    return 0;
}
