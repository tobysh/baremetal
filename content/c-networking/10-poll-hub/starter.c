#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int hub_step(int *fds, int n, int timeout_ms) {
    return -1;
}

// Optional: try it with Run (two clients on socketpairs).
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
