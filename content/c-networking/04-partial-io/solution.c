#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t read_full(int fd, void *buf, size_t n) {
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

ssize_t write_full(int fd, const void *buf, size_t n) {
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

int main(void) {
    int p[2];
    if (pipe(p) != 0) return 1;
    write_full(p[1], "hello", 5);
    close(p[1]);
    char buf[16];
    ssize_t r = read_full(p[0], buf, sizeof buf);
    printf("read %zd bytes (expected 5)\n", r);
    return 0;
}
