#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static struct sockaddr_in loopback(uint16_t port) {
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return a;
}

int udp_bind(uint16_t port, uint16_t *bound_port) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;
    struct sockaddr_in a = loopback(port);
    if (bind(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
    socklen_t len = sizeof a;
    if (getsockname(s, (struct sockaddr *)&a, &len) < 0) { close(s); return -1; }
    *bound_port = ntohs(a.sin_port);
    return s;
}

int udp_send(uint16_t port, const void *buf, size_t n) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;
    struct sockaddr_in a = loopback(port);
    ssize_t w = sendto(s, buf, n, 0, (struct sockaddr *)&a, sizeof a);
    close(s);
    return w == (ssize_t)n ? (int)n : -1;
}

int upper_reply_once(int fd, uint16_t *from_port) {
    char buf[512];
    struct sockaddr_in from;
    socklen_t fl = sizeof from;
    ssize_t got = recvfrom(fd, buf, sizeof buf, 0, (struct sockaddr *)&from, &fl);
    if (got < 0) return -1;
    for (ssize_t i = 0; i < got; i++) {
        if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] = (char)(buf[i] - 'a' + 'A');
    }
    if (sendto(fd, buf, (size_t)got, 0, (struct sockaddr *)&from, fl) != got) return -1;
    *from_port = ntohs(from.sin_port);
    return (int)got;
}

int udp_request(uint16_t port, const void *req, size_t n, void *resp, size_t cap, int timeout_ms) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;
    struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    int result = -1;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0) goto done;
    struct sockaddr_in a = loopback(port);
    if (sendto(s, req, n, 0, (struct sockaddr *)&a, sizeof a) != (ssize_t)n) goto done;
    ssize_t got = recv(s, resp, cap, 0);
    if (got >= 0) result = (int)got;
done:
    close(s);
    return result;
}

int main(void) {
    uint16_t port = 0, from = 0;
    int s = udp_bind(0, &port);
    if (s < 0) { printf("udp_bind failed\n"); return 1; }
    udp_send(port, "hello udp", 9);
    int n = upper_reply_once(s, &from);
    printf("served %d bytes from port %u\n", n, from);
    return 0;
}
