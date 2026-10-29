#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

int set_nonblocking(int fd) {
    return -1;
}

ssize_t drain(int fd, char *buf, size_t cap) {
    return -1;
}

ssize_t push(int fd, const char *buf, size_t n) {
    return -1;
}

int try_accept(int listen_fd) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    int sp[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    set_nonblocking(sp[0]);
    char buf[16];
    printf("drain on an empty socket: %zd (expected 0)\n", drain(sp[0], buf, sizeof buf));
    return 0;
}
