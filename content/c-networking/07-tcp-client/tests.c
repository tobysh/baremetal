#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int nanosleep(const struct timespec *, struct timespec *);

static int listen_any(uint16_t *port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bind(s, (struct sockaddr *)&a, sizeof a);
    listen(s, 8);
    socklen_t l = sizeof a;
    getsockname(s, (struct sockaddr *)&a, &l);
    *port = ntohs(a.sin_port);
    return s;
}

// Fork a server that answers `rounds` connections. Each reply is "<prefix><upper-cased request>\n"
// sent `piece` bytes at a time with a short pause between pieces. If `noeol` is set, no newline is sent
// and the connection is simply closed after the reply.
static pid_t spawn_server(int ls, int rounds, const char *prefix, int piece, int noeol) {
    pid_t pid = fork();
    if (pid != 0) return pid;
    for (int r = 0; r < rounds; r++) {
        int c = accept(ls, NULL, NULL);
        if (c < 0) _exit(2);
        char line[512];
        size_t n = 0;
        char ch;
        while (n < sizeof line - 1 && read(c, &ch, 1) == 1 && ch != '\n') line[n++] = ch;
        line[n] = 0;
        for (size_t i = 0; i < n; i++) if (line[i] >= 'a' && line[i] <= 'z') line[i] -= 32;
        char out[640];
        int m = snprintf(out, sizeof out, "%s%s%s", prefix, line, noeol ? "" : "\n");
        for (int i = 0; i < m; i += piece) {
            int k = m - i < piece ? m - i : piece;
            if (write(c, out + i, k) != k) break;
            struct timespec ts = {0, 2000000};
            nanosleep(&ts, NULL);
        }
        close(c);
    }
    _exit(0);
}

int main(void) {
    alarm(8);
    signal(SIGPIPE, SIG_IGN);
    uint16_t port;
    int ls = listen_any(&port);

    int fd = tcp_connect(port);
    CHECK("tcp_connect connects to a listening port", fd >= 0, "returned %d", fd);
    if (fd >= 0) {
        struct sockaddr_in peer;
        socklen_t pl = sizeof peer;
        int ok = getpeername(fd, (struct sockaddr *)&peer, &pl) == 0 && ntohs(peer.sin_port) == port && peer.sin_addr.s_addr == htonl(INADDR_LOOPBACK);
        CHECK("the socket's peer is 127.0.0.1 and that port", ok, "wrong peer");
        int c = accept(ls, NULL, NULL);
        CHECK("the server side sees the connection", c >= 0, "accept failed");
        if (c >= 0) close(c);
        close(fd);
    }

    uint16_t dead;
    int tmp = listen_any(&dead);
    close(tmp);
    int before = open("/dev/null", 0);
    close(before);
    CHECK("tcp_connect to a closed port fails with -1", tcp_connect(dead) == -1, "expected -1");
    int leaked = 0;
    for (int i = 0; i < 70; i++) tcp_connect(dead);
    int after = open("/dev/null", 0);
    leaked = after - before;
    close(after);
    CHECK("failed connects do not leak descriptors", leaked == 0, "descriptor numbers grew by %d (limit is 64 open files)", leaked);

    char reply[128];
    memset(reply, 'Z', sizeof reply);
    pid_t pid = spawn_server(ls, 1, "OK ", 2, 0);
    int n = line_request(port, "hello", reply, sizeof reply);
    CHECK("line_request returns the reply length", n == 8, "returned %d", n);
    CHECK("the reply has no newline and is NUL-terminated", strcmp(reply, "OK HELLO") == 0, "reply is \"%s\"", reply);
    waitpid(pid, NULL, 0);

    pid = spawn_server(ls, 1, "", 1, 0);
    n = line_request(port, "ping pong", reply, sizeof reply);
    CHECK("a reply sent one byte at a time is assembled", n == 9 && strcmp(reply, "PING PONG") == 0, "returned %d, reply \"%s\"", n, reply);
    waitpid(pid, NULL, 0);

    pid = spawn_server(ls, 1, "", 3, 0);
    n = line_request(port, "", reply, sizeof reply);
    CHECK("an empty request still works and an empty reply is length 0", n == 0 && reply[0] == '\0', "returned %d", n);
    waitpid(pid, NULL, 0);

    pid = spawn_server(ls, 1, "BYE ", 4, 1);
    n = line_request(port, "x", reply, sizeof reply);
    CHECK("a reply ended by close instead of newline is accepted", n == 5 && strcmp(reply, "BYE X") == 0, "returned %d, reply \"%s\"", n, reply);
    waitpid(pid, NULL, 0);

    pid = spawn_server(ls, 1, "", 50, 0);
    char tiny[8];
    n = line_request(port, "this reply is too long", tiny, sizeof tiny);
    CHECK("a reply that does not fit in cap returns -1", n == -1, "returned %d", n);
    waitpid(pid, NULL, 0);

    pid = spawn_server(ls, 1, "", 50, 0);
    char exact[6];
    n = line_request(port, "abcde", exact, sizeof exact);
    CHECK("a reply of cap-1 characters fits exactly", n == 5 && strcmp(exact, "ABCDE") == 0, "returned %d", n);
    waitpid(pid, NULL, 0);

    pid = spawn_server(ls, 3, "#", 2, 0);
    int all = 1;
    const char *rq[] = {"a", "bb", "ccc"};
    const char *rp[] = {"#A", "#BB", "#CCC"};
    for (int i = 0; i < 3; i++) {
        n = line_request(port, rq[i], reply, sizeof reply);
        if (n != (int)strlen(rp[i]) || strcmp(reply, rp[i]) != 0) all = 0;
    }
    CHECK("three requests in a row each get their own reply", all, "a reply was wrong");
    waitpid(pid, NULL, 0);

    n = line_request(dead, "x", reply, sizeof reply);
    CHECK("line_request to a closed port returns -1", n == -1, "returned %d", n);
    close(ls);
    return 0;
}
