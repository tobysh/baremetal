#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

static inline void put_u32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static inline uint32_t get_u32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

int make_channel(int fds[2]) {
    return -1;
}

int server_step(int fd) {
    return -1;
}

int client_call(int fd, uint32_t x, uint32_t *result) {
    return -1;
}

// Optional: try them with Run.
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
