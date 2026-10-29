#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t read_full(int fd, void *buf, size_t n) {
    // Currently reads once, which is the bug this lesson is about.
    return read(fd, buf, n);
}

ssize_t write_full(int fd, const void *buf, size_t n) {
    return write(fd, buf, n);
}

// Optional: try them with Run.
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
