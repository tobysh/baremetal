#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

static void p32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static uint32_t g32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

int main(void) {
    alarm(4);
    int fds[2] = {-1, -1};
    int r = make_channel(fds);
    CHECK("make_channel returns 0", r == 0, "returned %d", r);
    if (r != 0) return 0;
    int type = 0;
    socklen_t tl = sizeof type;
    getsockopt(fds[0], SOL_SOCKET, SO_TYPE, &type, &tl);
    CHECK("the pair is a stream socket", type == SOCK_STREAM, "type is %d", type);
    struct sockaddr_storage ss;
    socklen_t sl = sizeof ss;
    getsockname(fds[0], (struct sockaddr *)&ss, &sl);
    CHECK("the pair is AF_UNIX", ss.ss_family == AF_UNIX, "family is %d", ss.ss_family);
    uint8_t ping[1] = {9}, pong[1] = {0};
    CHECK("bytes cross from fds[0] to fds[1]", write(fds[0], ping, 1) == 1 && read(fds[1], pong, 1) == 1 && pong[0] == 9, "no data");

    uint32_t vals[] = {0, 1, 21, 1000, 0x7FFFFFFFu, 0x80000001u, 0xFFFFFFFFu};
    uint32_t want[] = {0, 2, 42, 2000, 0xFFFFFFFEu, 2, 0xFFFFFFFEu};
    int ok = 1;
    for (int i = 0; i < 7; i++) {
        uint8_t q[4], a[4] = {0};
        p32(q, vals[i]);
        write(fds[0], q, 4);
        int s = server_step(fds[1]);
        ssize_t n = read(fds[0], a, 4);
        if (s != 1 || n != 4 || g32(a) != want[i]) { ok = 0; printf("# input %u: step=%d n=%zd got %u want %u\n", vals[i], s, n, g32(a), want[i]); }
    }
    CHECK("server_step doubles the number (with 32-bit wrap-around)", ok, "see output above");
    int s2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, s2);
    close(s2[0]);
    r = server_step(s2[1]);
    CHECK("server_step returns 0 when the peer closed", r == 0, "returned %d", r);
    close(s2[1]);
    r = server_step(-1);
    CHECK("server_step returns -1 on a bad descriptor", r == -1, "returned %d", r);

    int c[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, c);
    pid_t pid = fork();
    if (pid == 0) {
        close(c[0]);
        while (server_step(c[1]) > 0) {}
        _exit(0);
    }
    close(c[1]);
    uint32_t res = 0;
    r = client_call(c[0], 7, &res);
    CHECK("client_call returns 0 and the doubled value", r == 0 && res == 14, "returned %d, result %u", r, res);
    r = client_call(c[0], 100000, &res);
    CHECK("client_call works repeatedly on one connection", r == 0 && res == 200000, "returned %d, result %u", r, res);
    r = client_call(c[0], 0xC0000000u, &res);
    CHECK("client_call passes large values through", r == 0 && res == 0x80000000u, "returned %d, result 0x%x", r, res);
    close(c[0]);
    int st = 0;
    waitpid(pid, &st, 0);

    int d[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, d);
    close(d[1]);
    signal(SIGPIPE, SIG_IGN);
    res = 123;
    r = client_call(d[0], 5, &res);
    CHECK("client_call returns -1 when the server is gone", r == -1, "returned %d", r);
    return 0;
}
