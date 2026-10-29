#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

int tcp_connect(uint16_t port) {
    return -1;
}

int line_request(uint16_t port, const char *request, char *reply, size_t cap) {
    return -1;
}

// Optional: Run it against a port where something listens, e.g. one you open yourself.
int main(void) {
    char reply[64];
    int n = line_request(9, "hello", reply, sizeof reply);
    printf("line_request returned %d (no server on port 9, so -1 is correct)\n", n);
    return 0;
}
