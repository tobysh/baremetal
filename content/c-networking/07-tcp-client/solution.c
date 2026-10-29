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
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) {
        close(s);
        return -1;
    }
    return s;
}

int line_request(uint16_t port, const char *request, char *reply, size_t cap) {
    int s = tcp_connect(port);
    if (s < 0) return -1;
    int result = -1;
    if (write_full(s, request, strlen(request)) < 0 || write_full(s, "\n", 1) < 0) goto done;
    size_t len = 0;
    for (;;) {
        char ch;
        ssize_t r = read(s, &ch, 1);
        if (r < 0) {
            if (errno == EINTR) continue;
            goto done;
        }
        if (r == 0 || ch == '\n') break;
        if (len + 1 >= cap) goto done;
        reply[len++] = ch;
    }
    if (cap == 0) goto done;
    reply[len] = '\0';
    result = (int)len;
done:
    close(s);
    return result;
}

int main(void) {
    char reply[64];
    int n = line_request(9, "hello", reply, sizeof reply);
    printf("line_request returned %d (no server on port 9, so -1 is correct)\n", n);
    return 0;
}
