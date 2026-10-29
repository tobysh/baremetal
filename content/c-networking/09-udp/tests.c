#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static struct sockaddr_in lo(uint16_t port) {
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return a;
}

static int plain_udp_server(uint16_t *port) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a = lo(0);
    bind(s, (struct sockaddr *)&a, sizeof a);
    socklen_t l = sizeof a;
    getsockname(s, (struct sockaddr *)&a, &l);
    *port = ntohs(a.sin_port);
    return s;
}

int main(void) {
    alarm(8);
    signal(SIGPIPE, SIG_IGN);
    uint16_t port = 0;
    int s = udp_bind(0, &port);
    CHECK("udp_bind returns a descriptor and a port", s >= 0 && port != 0, "returned %d, port %u", s, port);
    if (s < 0) return 0;
    int type = 0;
    socklen_t tl = sizeof type;
    getsockopt(s, SOL_SOCKET, SO_TYPE, &type, &tl);
    CHECK("the socket is a datagram socket", type == SOCK_DGRAM, "type is %d", type);
    struct timeval tv = {1, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    int n = udp_send(port, "ab", 2);
    CHECK("udp_send returns the byte count", n == 2, "returned %d", n);
    n = udp_send(port, "XYZ", 3);
    char buf[64];
    ssize_t got = recv(s, buf, sizeof buf, 0);
    CHECK("the first datagram arrives as its own message", got == 2 && memcmp(buf, "ab", 2) == 0, "got %zd bytes", got);
    got = recv(s, buf, sizeof buf, 0);
    CHECK("the second datagram arrives separately", got == 3 && memcmp(buf, "XYZ", 3) == 0, "got %zd bytes", got);
    CHECK("udp_send of an empty datagram returns 0", udp_send(port, "", 0) == 0, "expected 0");
    got = recv(s, buf, sizeof buf, 0);
    CHECK("an empty datagram arrives as 0 bytes", got == 0, "got %zd", got);

    // upper_reply_once: a plain client talks to the learner's server code.
    int c = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    struct sockaddr_in dest = lo(port);
    const char *msgs[] = {"hello, world 42", "ALREADY UPPER", "mIxEd cAsE", ""};
    const char *want[] = {"HELLO, WORLD 42", "ALREADY UPPER", "MIXED CASE", ""};
    struct sockaddr_in me;
    socklen_t ml = sizeof me;
    sendto(c, "x", 1, 0, (struct sockaddr *)&dest, sizeof dest);
    getsockname(c, (struct sockaddr *)&me, &ml);
    uint16_t myport = ntohs(me.sin_port);
    uint16_t from = 0;
    n = upper_reply_once(s, &from);
    got = recv(c, buf, sizeof buf, 0);
    CHECK("the reply goes back to the sender", n == 1 && got == 1 && buf[0] == 'X', "returned %d, client received %zd bytes", n, got);
    CHECK("from_port is the sender's port", from == myport, "from_port %u, sender %u", from, myport);
    int all = 1;
    for (int i = 0; i < 4; i++) {
        size_t len = strlen(msgs[i]);
        sendto(c, msgs[i], len, 0, (struct sockaddr *)&dest, sizeof dest);
        n = upper_reply_once(s, &from);
        memset(buf, '?', sizeof buf);
        got = recv(c, buf, sizeof buf, 0);
        if (n != (int)len || got != (ssize_t)len || memcmp(buf, want[i], len) != 0) { all = 0; printf("# \"%s\": served %d, got %zd\n", msgs[i], n, got); }
    }
    CHECK("letters are upper-cased, other bytes unchanged, empty datagrams work", all, "see output above");
    char long_msg[300];
    for (int i = 0; i < 300; i++) long_msg[i] = (char)('a' + i % 26);
    sendto(c, long_msg, 300, 0, (struct sockaddr *)&dest, sizeof dest);
    n = upper_reply_once(s, &from);
    char lb[400];
    got = recv(c, lb, sizeof lb, 0);
    CHECK("a 300 byte datagram is handled", n == 300 && got == 300 && lb[0] == 'A' && lb[299] == 'A' + 299 % 26, "returned %d, got %zd", n, got);
    close(c);

    // udp_request against a plain server in a child process.
    uint16_t sp;
    int ss = plain_udp_server(&sp);
    pid_t pid = fork();
    if (pid == 0) {
        for (int i = 0; i < 2; i++) {
            char b[256];
            struct sockaddr_in f;
            socklen_t fl = sizeof f;
            ssize_t g = recvfrom(ss, b, sizeof b, 0, (struct sockaddr *)&f, &fl);
            if (g < 0) _exit(1);
            char out[300];
            int m = snprintf(out, sizeof out, "echo:%.*s", (int)g, b);
            sendto(ss, out, m, 0, (struct sockaddr *)&f, fl);
        }
        _exit(0);
    }
    char resp[64];
    n = udp_request(sp, "ping", 4, resp, sizeof resp, 1000);
    CHECK("udp_request returns the reply length", n == 9 && memcmp(resp, "echo:ping", 9) == 0, "returned %d", n);
    n = udp_request(sp, "second", 6, resp, sizeof resp, 1000);
    CHECK("udp_request can be called again", n == 11 && memcmp(resp, "echo:second", 11) == 0, "returned %d", n);
    waitpid(pid, NULL, 0);
    close(ss);

    // Nobody answers: must time out, not hang.
    uint16_t qp;
    int qs = plain_udp_server(&qp);
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    n = udp_request(qp, "hello?", 6, resp, sizeof resp, 200);
    gettimeofday(&t1, NULL);
    long ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_usec - t0.tv_usec) / 1000;
    CHECK("udp_request returns -1 when nobody replies", n == -1, "returned %d", n);
    CHECK("it gives up after about timeout_ms", ms >= 150 && ms < 1500, "waited %ld ms for a 200 ms timeout", ms);
    close(qs);

    // Descriptor hygiene.
    int before = open("/dev/null", 0);
    close(before);
    for (int i = 0; i < 70; i++) udp_request(qp, "z", 1, resp, sizeof resp, 1);
    int after = open("/dev/null", 0);
    close(after);
    CHECK("udp_request closes its socket on every path", after == before, "descriptor numbers grew by %d", after - before);
    close(s);
    return 0;
}
