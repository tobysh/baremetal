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

int udp_bind(uint16_t port, uint16_t *bound_port) {
    return -1;
}

int udp_send(uint16_t port, const void *buf, size_t n) {
    return -1;
}

int upper_reply_once(int fd, uint16_t *from_port) {
    return -1;
}

int udp_request(uint16_t port, const void *req, size_t n, void *resp, size_t cap, int timeout_ms) {
    return -1;
}

// Optional: try them with Run.
int main(void) {
    uint16_t port = 0, from = 0;
    int s = udp_bind(0, &port);
    if (s < 0) { printf("udp_bind failed\n"); return 1; }
    udp_send(port, "hello udp", 9);
    int n = upper_reply_once(s, &from);
    printf("served %d bytes from port %u\n", n, from);
    return 0;
}
