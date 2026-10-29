#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int dial(uint16_t port) {
    int c = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(c, (struct sockaddr *)&a, sizeof a) < 0) { close(c); return -1; }
    struct timeval tv = {1, 0};  // never wait forever for a reply
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    return c;
}

int main(void) {
    alarm(5);
    signal(SIGPIPE, SIG_IGN);
    uint16_t port = 0;
    int ls = tcp_listen(0, &port);
    CHECK("tcp_listen returns a descriptor", ls >= 0, "returned %d", ls);
    if (ls < 0) return 0;
    CHECK("tcp_listen reports the chosen port", port != 0, "port is 0");

    struct sockaddr_in a;
    socklen_t al = sizeof a;
    getsockname(ls, (struct sockaddr *)&a, &al);
    CHECK("the socket is bound to the reported port", ntohs(a.sin_port) == port, "bound to %u, reported %u", ntohs(a.sin_port), port);
    CHECK("the socket is bound to 127.0.0.1", a.sin_addr.s_addr == htonl(INADDR_LOOPBACK), "address is %s", inet_ntoa(a.sin_addr));
    int acc = 0;
    socklen_t sl = sizeof acc;
    getsockopt(ls, SOL_SOCKET, SO_ACCEPTCONN, &acc, &sl);
    CHECK("the socket is listening", acc != 0, "SO_ACCEPTCONN is 0");
    int ra = 0;
    sl = sizeof ra;
    getsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &ra, &sl);
    CHECK("SO_REUSEADDR is set", ra != 0, "not set");

    const char *msgs[] = {"hello", "x", "The quick brown fox jumps over the lazy dog."};
    for (int m = 0; m < 3; m++) {
        int c = dial(port);
        size_t n = strlen(msgs[m]);
        write(c, msgs[m], n);
        shutdown(c, SHUT_WR);
        long served = echo_serve_one(ls);
        char buf[128] = {0};
        ssize_t got = 0, r;
        while ((r = read(c, buf + got, sizeof buf - 1 - got)) > 0) got += r;
        char name[64];
        snprintf(name, sizeof name, "connection %d: the client gets its bytes back", m + 1);
        CHECK(name, (size_t)got == n && memcmp(buf, msgs[m], n) == 0, "got %zd bytes: \"%s\"", got, buf);
        snprintf(name, sizeof name, "connection %d: echo_serve_one returns the byte count", m + 1);
        CHECK(name, served == (long)n, "returned %ld, expected %zu", served, n);
        close(c);
    }

    int c = dial(port);
    shutdown(c, SHUT_WR);
    long served = echo_serve_one(ls);
    char b1[4];
    CHECK("a client that sends nothing gets 0 and EOF", served == 0 && read(c, b1, sizeof b1) == 0, "returned %ld", served);
    close(c);

    enum { N = 6000 };
    char *big = malloc(N), *back = malloc(N);
    for (int i = 0; i < N; i++) big[i] = (char)('a' + (i * 7 + i / 13) % 26);
    c = dial(port);
    write(c, big, N);
    shutdown(c, SHUT_WR);
    served = echo_serve_one(ls);
    ssize_t got = 0, r;
    while (got < N && (r = read(c, back + got, N - got)) > 0) got += r;
    CHECK("a 6000 byte message is echoed intact", served == N && got == N && memcmp(big, back, N) == 0, "served %ld, client got %zd", served, got);
    close(c);

    close(ls);
    uint16_t again = 0;
    int ls2 = tcp_listen(port, &again);
    CHECK("tcp_listen can bind a specific port", ls2 >= 0 && again == port, "returned %d, port %u (wanted %u)", ls2, again, port);
    if (ls2 >= 0) {
        int c2 = dial(port);
        CHECK("a client can connect to that port", c2 >= 0, "connect failed");
        if (c2 >= 0) close(c2);
        close(ls2);
    }
    free(big);
    free(back);
    return 0;
}
