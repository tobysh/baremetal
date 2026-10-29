#include <errno.h>
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

int tcp_listen(uint16_t port, uint16_t *bound_port) {
    return -1;
}

long echo_serve_one(int listen_fd) {
    return -1;
}

// Optional: try it with Run (a client in the same process).
int main(void) {
    uint16_t port = 0;
    int ls = tcp_listen(0, &port);
    if (ls < 0) { printf("tcp_listen failed\n"); return 1; }
    printf("listening on 127.0.0.1:%u\n", port);
    int c = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(c, (struct sockaddr *)&a, sizeof a);
    write(c, "hello", 5);
    shutdown(c, SHUT_WR);
    printf("echoed %ld bytes (expected 5)\n", echo_serve_one(ls));
    char buf[16] = {0};
    read(c, buf, sizeof buf - 1);
    printf("client got \"%s\"\n", buf);
    return 0;
}
