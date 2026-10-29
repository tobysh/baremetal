#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>

int nanosleep(const struct timespec *, struct timespec *);
int siginterrupt(int, int);
static void nap(int ms) { struct timespec ts = {0, ms * 1000000L}; nanosleep(&ts, NULL); }
// Strict C17 mode gives signal() one-shot System V semantics, so the handler re-arms itself.
static void on_alarm(int s) { signal(s, on_alarm); }

int main(void) {
    alarm(8);
    signal(SIGPIPE, SIG_IGN);
    unsigned char data[100], got[100];
    for (int i = 0; i < 100; i++) data[i] = (unsigned char)(i * 7 + 3);

    // 1. Data trickling in, 1 to 3 bytes at a time.
    int sp[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    pid_t pid = fork();
    if (pid == 0) {
        close(sp[0]);
        int i = 0, chunk = 1;
        while (i < 100) {
            int m = i + chunk > 100 ? 100 - i : chunk;
            if (write(sp[1], data + i, m) != m) _exit(1);
            i += m;
            chunk = chunk % 3 + 1;
            nap(1);
        }
        close(sp[1]);
        _exit(0);
    }
    close(sp[1]);
    memset(got, 0, sizeof got);
    ssize_t r = read_full(sp[0], got, 100);
    CHECK("read_full collects 100 bytes arriving in tiny pieces", r == 100 && memcmp(got, data, 100) == 0, "returned %zd", r);
    r = read_full(sp[0], got, 10);
    CHECK("read_full returns 0 at end of file", r == 0, "returned %zd", r);
    close(sp[0]);
    waitpid(pid, NULL, 0);

    // 2. EOF before n bytes.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    write(sp[1], data, 37);
    close(sp[1]);
    memset(got, 0, sizeof got);
    r = read_full(sp[0], got, 100);
    CHECK("read_full returns the short count when the peer closes early", r == 37 && memcmp(got, data, 37) == 0, "returned %zd", r);
    close(sp[0]);

    // 3. Does not over-read: only n bytes are consumed.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    write(sp[1], data, 50);
    r = read_full(sp[0], got, 20);
    unsigned char rest[50];
    ssize_t r2 = read(sp[0], rest, 50);
    CHECK("read_full reads exactly n, leaving the rest", r == 20 && r2 == 30 && memcmp(rest, data + 20, 30) == 0, "returned %zd, then %zd", r, r2);
    close(sp[0]);
    close(sp[1]);

    // 4. A signal interrupts the blocked read.
    signal(SIGALRM, on_alarm);
    siginterrupt(SIGALRM, 1);  // deliver EINTR instead of restarting the call
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    pid = fork();
    if (pid == 0) {
        close(sp[0]);
        nap(120);
        write(sp[1], data, 10);
        nap(60);
        write(sp[1], data + 10, 10);
        _exit(0);
    }
    close(sp[1]);
    struct itimerval it = {{0, 10000}, {0, 10000}};
    setitimer(ITIMER_REAL, &it, NULL);
    memset(got, 0, sizeof got);
    r = read_full(sp[0], got, 20);
    struct itimerval off = {{0, 0}, {0, 0}};
    setitimer(ITIMER_REAL, &off, NULL);
    CHECK("read_full survives EINTR and keeps waiting", r == 20 && memcmp(got, data, 20) == 0, "returned %zd, errno %d", r, errno);
    close(sp[0]);
    waitpid(pid, NULL, 0);

    // 5. A big write through a small buffer needs a loop.
    enum { BIG = 1 << 20 };
    unsigned char *big = malloc(BIG);
    for (int i = 0; i < BIG; i++) big[i] = (unsigned char)(i * 31 + (i >> 8));
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    int small = 4096;
    setsockopt(sp[0], SOL_SOCKET, SO_SNDBUF, &small, sizeof small);
    pid = fork();
    if (pid == 0) {
        close(sp[0]);
        unsigned char *in = malloc(BIG);
        size_t have = 0;
        while (have < BIG) {
            ssize_t k = read(sp[1], in + have, BIG - have > 1500 ? 1500 : BIG - have);
            if (k <= 0) break;
            have += k;
            if (have % 65536 < 1500) nap(1);
        }
        _exit(have == BIG && memcmp(in, big, BIG) == 0 ? 0 : 1);
    }
    close(sp[1]);
    struct itimerval it2 = {{0, 7000}, {0, 7000}};
    setitimer(ITIMER_REAL, &it2, NULL);
    r = write_full(sp[0], big, BIG);
    setitimer(ITIMER_REAL, &off, NULL);
    CHECK("write_full returns n after a 1 MiB write", r == BIG, "returned %zd", r);
    close(sp[0]);
    int st = 0;
    waitpid(pid, &st, 0);
    CHECK("the receiver got every byte intact", WIFEXITED(st) && WEXITSTATUS(st) == 0, "receiver status %d", st);

    r = write_full(0, "", 0);
    CHECK("write_full of zero bytes returns 0", r == 0, "returned %zd", r);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    close(sp[1]);
    r = write_full(sp[0], data, 10);
    CHECK("write_full returns -1 when the peer is gone", r == -1, "returned %zd", r);
    r = read_full(-1, got, 5);
    CHECK("read_full returns -1 on a bad descriptor", r == -1, "returned %zd", r);
    free(big);
    return 0;
}
