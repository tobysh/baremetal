#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int hub_step(int *fds, int n, int timeout_ms) {
    if (n <= 0) return 0;
    struct pollfd p[n];
    for (int i = 0; i < n; i++) {
        p[i].fd = fds[i];
        p[i].events = POLLIN;
        p[i].revents = 0;
    }
    int ready;
    do {
        ready = poll(p, (nfds_t)n, timeout_ms);
    } while (ready < 0 && errno == EINTR);
    if (ready < 0) return -1;
    if (ready == 0) return 0;

    int relayed = 0;
    for (int i = 0; i < n; i++) {
        if (fds[i] < 0 || !(p[i].revents & (POLLIN | POLLHUP | POLLERR))) continue;
        char buf[256];
        ssize_t k = read(fds[i], buf, sizeof buf);
        if (k <= 0) {
            close(fds[i]);
            fds[i] = -1;
            continue;
        }
        for (int j = 0; j < n; j++) {
            if (j == i || fds[j] < 0) continue;
            size_t done = 0;
            while (done < (size_t)k) {
                ssize_t w = write(fds[j], buf + done, (size_t)k - done);
                if (w < 0) {
                    if (errno == EINTR) continue;
                    break;
                }
                done += (size_t)w;
            }
        }
        relayed++;
    }
    return relayed;
}

int main(void) {
    int a[2], b[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, a);
    socketpair(AF_UNIX, SOCK_STREAM, 0, b);
    int hub[2] = {a[1], b[1]};
    write(a[0], "hi", 2);
    printf("relayed %d message(s) (expected 1)\n", hub_step(hub, 2, 500));
    char buf[8] = {0};
    read(b[0], buf, sizeof buf - 1);
    printf("client B received \"%s\"\n", buf);
    return 0;
}
