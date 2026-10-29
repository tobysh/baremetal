#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int nanosleep(const struct timespec *, struct timespec *);
static void nap(int ms) { struct timespec ts = {0, ms * 1000000L}; nanosleep(&ts, NULL); }

// Child: write raw bytes a few at a time, then close.
static pid_t dribble(int fd, const uint8_t *data, size_t n, size_t piece, int closefd) {
    pid_t pid = fork();
    if (pid != 0) return pid;
    if (closefd >= 0) close(closefd);
    for (size_t i = 0; i < n; i += piece) {
        size_t k = n - i < piece ? n - i : piece;
        if (write(fd, data + i, k) != (ssize_t)k) _exit(1);
        nap(1);
    }
    close(fd);
    _exit(0);
}

int main(void) {
    alarm(8);
    signal(SIGPIPE, SIG_IGN);
    int sp[2];
    uint8_t buf[2048], raw[2048];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);

    int r = send_frame(sp[0], (const uint8_t *)"hello", 5);
    ssize_t got = read(sp[1], raw, sizeof raw);
    CHECK("send_frame returns 0", r == 0, "returned %d", r);
    CHECK("a frame is a big-endian length then the payload", got == 7 && raw[0] == 0 && raw[1] == 5 && memcmp(raw + 2, "hello", 5) == 0, "got %zd bytes: %02x %02x ...", got, raw[0], raw[1]);
    r = send_frame(sp[0], (const uint8_t *)"", 0);
    got = read(sp[1], raw, sizeof raw);
    CHECK("an empty frame is just a zero length", r == 0 && got == 2 && raw[0] == 0 && raw[1] == 0, "returned %d, got %zd bytes", r, got);
    uint8_t big[1025];
    memset(big, 'q', sizeof big);
    r = send_frame(sp[0], big, 1024);
    got = 0;
    ssize_t k;
    while (got < 1026 && (k = read(sp[1], raw + got, sizeof raw - got)) > 0) got += k;
    CHECK("a maximum size frame is sent whole", r == 0 && got == 1026 && raw[0] == 0x04 && raw[1] == 0x00, "returned %d, %zd bytes, header %02x %02x", r, got, raw[0], raw[1]);
    r = send_frame(sp[0], big, 1025);
    nap(5);
    k = recv(sp[1], raw, sizeof raw, MSG_DONTWAIT);
    CHECK("an oversized frame is refused and nothing is sent", r == -1 && k < 0, "returned %d and %zd bytes appeared", r, k);
    close(sp[0]);
    close(sp[1]);

    // recv_frame with data arriving in tiny pieces.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    uint8_t wire[] = {0, 5, 'h', 'e', 'l', 'l', 'o', 0, 0, 0, 3, 'a', 0, 'c'};
    pid_t pid = dribble(sp[1], wire, sizeof wire, 1, sp[0]);
    close(sp[1]);
    memset(buf, 0, sizeof buf);
    r = recv_frame(sp[0], buf, sizeof buf);
    CHECK("recv_frame assembles a frame sent a byte at a time", r == 5 && memcmp(buf, "hello", 5) == 0, "returned %d", r);
    r = recv_frame(sp[0], buf, sizeof buf);
    CHECK("recv_frame returns 0 for an empty frame", r == 0, "returned %d", r);
    r = recv_frame(sp[0], buf, sizeof buf);
    CHECK("payload bytes may be zero (binary safe)", r == 3 && buf[0] == 'a' && buf[1] == 0 && buf[2] == 'c', "returned %d", r);
    r = recv_frame(sp[0], buf, sizeof buf);
    CHECK("a clean close between frames returns -2", r == -2, "returned %d", r);
    close(sp[0]);
    waitpid(pid, NULL, 0);

    // Truncated frames.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    uint8_t cut[] = {0, 10, 'a', 'b', 'c'};
    write(sp[1], cut, sizeof cut);
    close(sp[1]);
    r = recv_frame(sp[0], buf, sizeof buf);
    CHECK("a close in the middle of a payload returns -1", r == -1, "returned %d", r);
    close(sp[0]);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    write(sp[1], cut, 1);
    close(sp[1]);
    r = recv_frame(sp[0], buf, sizeof buf);
    CHECK("a close in the middle of the header returns -1", r == -1, "returned %d", r);
    close(sp[0]);

    // Too big for the buffer / too big for the protocol.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    uint8_t wide[2 + 500];
    wide[0] = 0x01; wide[1] = 0xF4;
    memset(wide + 2, 'z', 500);
    write(sp[1], wide, sizeof wide);
    struct { uint8_t data[100]; uint8_t guard[100]; } sb;
    memset(&sb, 0x11, sizeof sb);
    r = recv_frame(sp[0], sb.data, 100);
    CHECK("a frame larger than cap returns -1", r == -1, "returned %d", r);
    int clean = 1;
    for (int i = 0; i < 100; i++) if (sb.guard[i] != 0x11) clean = 0;
    CHECK("recv_frame never writes past cap", clean, "the bytes after the buffer were overwritten");
    close(sp[0]);
    close(sp[1]);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    uint8_t huge[] = {0xFF, 0xFF, 1, 2, 3};
    write(sp[1], huge, sizeof huge);
    r = recv_frame(sp[0], buf, sizeof buf);
    CHECK("a length above MAX_FRAME returns -1", r == -1, "returned %d", r);
    close(sp[0]);
    close(sp[1]);

    // serve_reversed over a socketpair.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    pid = fork();
    if (pid == 0) {
        close(sp[0]);
        int n = serve_reversed(sp[1]);
        _exit(n == 4 ? 0 : 10 + n);
    }
    close(sp[1]);
    const char *words[] = {"stressed", "a", "", "Hello, frames!"};
    int ok = 1;
    for (int i = 0; i < 4; i++) {
        size_t n = strlen(words[i]);
        if (send_frame(sp[0], (const uint8_t *)words[i], n) != 0) { ok = 0; break; }
        int m = recv_frame(sp[0], buf, sizeof buf);
        if (m != (int)n) { ok = 0; break; }
        for (size_t j = 0; j < n; j++) if (buf[j] != (uint8_t)words[i][n - 1 - j]) ok = 0;
    }
    CHECK("serve_reversed replies with each payload reversed", ok, "a reply was wrong");
    close(sp[0]);
    int st = 0;
    waitpid(pid, &st, 0);
    CHECK("serve_reversed returns the frame count when the client closes", WIFEXITED(st) && WEXITSTATUS(st) == 0, "status %d (child exits 10+n when the count n is wrong)", WIFEXITED(st) ? WEXITSTATUS(st) : -1);

    // And over real TCP, the client dribbling its frames.
    int ls = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bind(ls, (struct sockaddr *)&a, sizeof a);
    listen(ls, 4);
    socklen_t al = sizeof a;
    getsockname(ls, (struct sockaddr *)&a, &al);
    pid = fork();
    if (pid == 0) {
        close(ls);
        int c = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(c, (struct sockaddr *)&a, sizeof a) < 0) _exit(1);
        uint8_t req[] = {0, 3, 'a', 'b', 'c', 0, 2, 'x', 'y'};
        for (size_t i = 0; i < sizeof req; i += 2) {
            size_t n = sizeof req - i < 2 ? sizeof req - i : 2;
            write(c, req + i, n);
            nap(2);
        }
        uint8_t resp[32];
        ssize_t t = 0, kk;
        while (t < 9 && (kk = read(c, resp + t, sizeof resp - t)) > 0) t += kk;
        _exit(t == 9 && resp[2] == 'c' && resp[3] == 'b' && resp[4] == 'a' && resp[7] == 'y' && resp[8] == 'x' ? 0 : 1);
    }
    int c = accept(ls, NULL, NULL);
    r = serve_reversed(c);
    close(c);
    waitpid(pid, &st, 0);
    CHECK("serve_reversed works on a real TCP connection", r == 2 && WIFEXITED(st) && WEXITSTATUS(st) == 0, "served %d frames, client status %d", r, st);
    close(ls);
    return 0;
}
