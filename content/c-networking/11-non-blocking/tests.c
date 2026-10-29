#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int is_nb(int fd) { int f = fcntl(fd, F_GETFL, 0); return f >= 0 && (f & O_NONBLOCK); }

int main(void) {
    alarm(5);
    signal(SIGPIPE, SIG_IGN);
    int sp[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);

    CHECK("a fresh descriptor is blocking", !is_nb(sp[0]), "already non-blocking?");
    int r = set_nonblocking(sp[0]);
    CHECK("set_nonblocking returns 0 and sets the flag", r == 0 && is_nb(sp[0]), "returned %d", r);
    int p[2];
    pipe(p);
    int fl = fcntl(p[0], F_GETFL, 0);
    fcntl(p[0], F_SETFL, fl | O_APPEND);
    set_nonblocking(p[0]);
    CHECK("existing flags are preserved", (fcntl(p[0], F_GETFL, 0) & O_APPEND) != 0 && is_nb(p[0]), "O_APPEND was lost");
    close(p[0]); close(p[1]);
    CHECK("set_nonblocking(-1) fails", set_nonblocking(-1) == -1, "expected -1");
    set_nonblocking(sp[1]);

    char buf[256];
    ssize_t n = drain(sp[0], buf, sizeof buf);
    CHECK("drain on an empty socket returns 0 without blocking", n == 0, "returned %zd", n);

    write(sp[1], "abc", 3);
    n = drain(sp[0], buf, sizeof buf);
    CHECK("drain returns what is available", n == 3 && memcmp(buf, "abc", 3) == 0, "returned %zd", n);
    n = drain(sp[0], buf, sizeof buf);
    CHECK("drain again returns 0 (EAGAIN is not an error)", n == 0, "returned %zd", n);

    write(sp[1], "0123456789", 10);
    write(sp[1], "ABCDEFGHIJ", 10);
    n = drain(sp[0], buf, sizeof buf);
    CHECK("drain collects several writes in one call", n == 20 && memcmp(buf, "0123456789ABCDEFGHIJ", 20) == 0, "returned %zd", n);

    char data[1000];
    for (int i = 0; i < 1000; i++) data[i] = (char)('A' + i % 26);
    write(sp[1], data, 1000);
    n = drain(sp[0], buf, 100);
    CHECK("drain stops at cap", n == 100 && memcmp(buf, data, 100) == 0, "returned %zd", n);
    char more[1000];
    n = drain(sp[0], more, sizeof more);
    CHECK("the rest is still there for the next call", n == 900 && memcmp(more, data + 100, 900) == 0, "returned %zd", n);

    write(sp[1], "bye", 3);
    close(sp[1]);
    n = drain(sp[0], buf, sizeof buf);
    CHECK("data before a close is returned first", n == 3 && memcmp(buf, "bye", 3) == 0, "returned %zd", n);
    n = drain(sp[0], buf, sizeof buf);
    CHECK("then drain reports the close with -2", n == -2, "returned %zd", n);
    close(sp[0]);

    // push against a full socket.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    int small = 4096;
    setsockopt(sp[0], SOL_SOCKET, SO_SNDBUF, &small, sizeof small);
    set_nonblocking(sp[0]);
    n = push(sp[0], "hello", 5);
    CHECK("push of a small message writes it all", n == 5, "returned %zd", n);
    n = push(sp[0], "", 0);
    CHECK("push of nothing returns 0", n == 0, "returned %zd", n);
    static char huge[4 << 20];
    for (size_t i = 0; i < sizeof huge; i++) huge[i] = (char)(i * 13 + (i >> 9));
    n = push(sp[0], huge, sizeof huge);
    CHECK("push returns a short count instead of blocking when the buffer fills", n > 0 && (size_t)n < sizeof huge, "returned %zd", n);
    ssize_t first = n;
    n = push(sp[0], huge + first, 1000);
    CHECK("pushing to a full socket accepts nothing (0)", n == 0, "returned %zd", n);

    static char got[4 << 20];
    size_t total = 0;
    ssize_t k;
    while ((k = recv(sp[1], got + total, sizeof got - total, MSG_DONTWAIT)) > 0) total += k;
    // "hello" came first, then the pushed prefix.
    CHECK("the receiver gets exactly the accepted bytes", total == (size_t)first + 5 && memcmp(got, "hello", 5) == 0 && memcmp(got + 5, huge, first) == 0, "receiver got %zu, push said %zd+5", total, first);
    n = push(sp[0], huge + first, 500);
    CHECK("after the receiver drained, push works again", n == 500, "returned %zd", n);
    close(sp[1]);
    n = push(sp[0], "x", 1);
    CHECK("push to a closed peer returns -1", n == -1, "returned %zd", n);
    close(sp[0]);

    // try_accept
    int ls = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bind(ls, (struct sockaddr *)&a, sizeof a);
    listen(ls, 8);
    socklen_t al = sizeof a;
    getsockname(ls, (struct sockaddr *)&a, &al);
    set_nonblocking(ls);
    r = try_accept(ls);
    CHECK("try_accept returns -2 when nobody is waiting", r == -2, "returned %d", r);
    int c1 = socket(AF_INET, SOCK_STREAM, 0), c2 = socket(AF_INET, SOCK_STREAM, 0);
    connect(c1, (struct sockaddr *)&a, sizeof a);
    connect(c2, (struct sockaddr *)&a, sizeof a);
    int s1 = try_accept(ls), s2 = try_accept(ls);
    CHECK("try_accept returns waiting clients one by one", s1 >= 0 && s2 >= 0 && s1 != s2, "returned %d and %d", s1, s2);
    CHECK("accepted sockets are non-blocking", s1 >= 0 && s2 >= 0 && is_nb(s1) && is_nb(s2), "flag missing");
    r = try_accept(ls);
    CHECK("then -2 again", r == -2, "returned %d", r);
    if (s1 >= 0) {
        write(c1, "hi", 2);
        char b[8];
        struct pollfd ready = { .fd = s1, .events = POLLIN };
        n = poll(&ready, 1, 1000) > 0 ? drain(s1, b, sizeof b) : -1;
        CHECK("an accepted socket is connected to the right client", n == 2 && memcmp(b, "hi", 2) == 0, "drain returned %zd", n);
    }
    r = try_accept(-1);
    CHECK("try_accept on a bad descriptor returns -1", r == -1, "returned %d", r);
    return 0;
}
