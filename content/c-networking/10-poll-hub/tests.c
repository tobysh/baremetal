#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

static long now_ms(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000L + t.tv_usec / 1000;
}

// Read whatever is there (non-blocking), return length.
static ssize_t slurp(int fd, char *buf, size_t cap) {
    ssize_t k = recv(fd, buf, cap - 1, MSG_DONTWAIT);
    if (k < 0) k = -1; else buf[k] = 0;
    return k;
}

int main(void) {
    alarm(10);
    signal(SIGPIPE, SIG_IGN);
    int client[3], hubside[3], fds[3];
    for (int i = 0; i < 3; i++) {
        int sp[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
        client[i] = sp[0];
        hubside[i] = fds[i] = sp[1];
    }
    char buf[300];

    long t0 = now_ms();
    int r = hub_step(fds, 3, 150);
    long dt = now_ms() - t0;
    CHECK("with nothing to read, hub_step returns 0", r == 0, "returned %d", r);
    CHECK("... after waiting about the timeout", dt >= 100 && dt < 1000, "waited %ld ms for a 150 ms timeout", dt);

    write(client[0], "hello", 5);
    t0 = now_ms();
    r = hub_step(fds, 3, 5000);
    dt = now_ms() - t0;
    CHECK("a message is relayed (returns 1)", r == 1, "returned %d", r);
    CHECK("hub_step returns promptly when data is ready", dt < 1000, "took %ld ms", dt);
    ssize_t k1 = slurp(client[1], buf, sizeof buf);
    CHECK("client 1 receives the bytes", k1 == 5 && strcmp(buf, "hello") == 0, "got %zd bytes", k1);
    ssize_t k2 = slurp(client[2], buf, sizeof buf);
    CHECK("client 2 receives the bytes", k2 == 5 && strcmp(buf, "hello") == 0, "got %zd bytes", k2);
    ssize_t k0 = slurp(client[0], buf, sizeof buf);
    CHECK("the sender does not get its own message back", k0 == -1, "client 0 received %zd bytes", k0);

    write(client[1], "AA", 2);
    write(client[2], "BBB", 3);
    r = hub_step(fds, 3, 2000);
    CHECK("two ready clients are both served in one step", r == 2, "returned %d", r);
    char b0[64], b1[64], b2[64];
    ssize_t g0 = slurp(client[0], b0, sizeof b0), g1 = slurp(client[1], b1, sizeof b1), g2 = slurp(client[2], b2, sizeof b2);
    CHECK("client 0 receives both messages", g0 == 5 && (strcmp(b0, "AABBB") == 0 || strcmp(b0, "BBBAA") == 0), "got %zd bytes: \"%s\"", g0, g0 > 0 ? b0 : "");
    CHECK("client 1 receives only client 2's message", g1 == 3 && strcmp(b1, "BBB") == 0, "got %zd bytes: \"%s\"", g1, g1 > 0 ? b1 : "");
    CHECK("client 2 receives only client 1's message", g2 == 2 && strcmp(b2, "AA") == 0, "got %zd bytes: \"%s\"", g2, g2 > 0 ? b2 : "");

    char big[600];
    memset(big, 'k', sizeof big);
    write(client[0], big, sizeof big);
    r = hub_step(fds, 3, 1000);
    ssize_t gk = slurp(client[1], buf, sizeof buf);
    CHECK("a read takes at most 256 bytes per step", r == 1 && gk == 256, "returned %d, relayed %zd bytes", r, gk);
    r = hub_step(fds, 3, 1000);
    gk = slurp(client[1], buf, sizeof buf);
    CHECK("the next step relays the next piece", r == 1 && gk == 256, "returned %d, relayed %zd bytes", r, gk);
    r = hub_step(fds, 3, 1000);
    gk = slurp(client[1], buf, sizeof buf);
    CHECK("... and the remaining 88 bytes", r == 1 && gk == 88, "returned %d, relayed %zd bytes", r, gk);
    while (slurp(client[2], buf, sizeof buf) > 0) {}

    // Client 1 hangs up.
    close(client[1]);
    r = hub_step(fds, 3, 1000);
    CHECK("a hang-up relays nothing (returns 0)", r == 0, "returned %d", r);
    CHECK("the closed slot becomes -1", fds[1] == -1, "fds[1] is %d", fds[1]);
    CHECK("the other slots are untouched", fds[0] == hubside[0] && fds[2] == hubside[2], "slots changed");
    CHECK("the hub really closed its end of the descriptor", write(hubside[1], "x", 1) == -1, "write to the old descriptor still works");

    write(client[0], "still here", 10);
    r = hub_step(fds, 3, 1000);
    k2 = slurp(client[2], buf, sizeof buf);
    CHECK("remaining clients keep chatting and -1 slots are skipped", r == 1 && k2 == 10 && strcmp(buf, "still here") == 0, "returned %d, client 2 got %zd bytes", r, k2);

    close(client[0]);
    close(client[2]);
    r = hub_step(fds, 3, 1000);
    CHECK("two hang-ups in one step close both slots", fds[0] == -1 && fds[2] == -1, "slots are %d %d %d", fds[0], fds[1], fds[2]);
    t0 = now_ms();
    r = hub_step(fds, 3, 100);
    dt = now_ms() - t0;
    CHECK("an all--1 array just times out", r == 0 && dt < 1000, "returned %d after %ld ms", r, dt);
    r = hub_step(fds, 0, 10);
    CHECK("n = 0 returns 0", r == 0, "returned %d", r);
    return 0;
}
