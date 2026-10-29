#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

int set_nonblocking(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0 ? -1 : 0;
}

ssize_t drain(int fd, char *buf, size_t cap) {
    size_t got = 0;
    int eof = 0;
    while (got < cap) {
        ssize_t r = read(fd, buf + got, cap - got);
        if (r > 0) {
            got += (size_t)r;
            continue;
        }
        if (r == 0) {
            eof = 1;
            break;
        }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        return -1;
    }
    if (eof && got == 0) return -2;
    return (ssize_t)got;
}

ssize_t push(int fd, const char *buf, size_t n) {
    size_t done = 0;
    while (done < n) {
        ssize_t w = write(fd, buf + done, n - done);
        if (w >= 0) {
            done += (size_t)w;
            continue;
        }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        return -1;
    }
    return (ssize_t)done;
}

int try_accept(int listen_fd) {
    int c;
    do {
        c = accept(listen_fd, NULL, NULL);
    } while (c < 0 && errno == EINTR);
    if (c < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
        return -1;
    }
    if (set_nonblocking(c) < 0) {
        close(c);
        return -1;
    }
    return c;
}

int main(void) {
    int sp[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    set_nonblocking(sp[0]);
    char buf[16];
    printf("drain on an empty socket: %zd (expected 0)\n", drain(sp[0], buf, sizeof buf));
    return 0;
}
