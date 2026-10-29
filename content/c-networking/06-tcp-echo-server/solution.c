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
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    int one = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0) { close(s); return -1; }
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
    if (listen(s, 16) < 0) { close(s); return -1; }
    socklen_t len = sizeof a;
    if (getsockname(s, (struct sockaddr *)&a, &len) < 0) { close(s); return -1; }
    *bound_port = ntohs(a.sin_port);
    return s;
}

long echo_serve_one(int listen_fd) {
    int c;
    do { c = accept(listen_fd, NULL, NULL); } while (c < 0 && errno == EINTR);
    if (c < 0) return -1;
    long total = 0;
    char buf[1024];
    ssize_t n;
    while ((n = read(c, buf, sizeof buf)) != 0) {
        if (n < 0) {
            if (errno == EINTR) continue;
            close(c);
            return -1;
        }
        if (write_full(c, buf, (size_t)n) < 0) { close(c); return -1; }
        total += n;
    }
    close(c);
    return total;
}

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
