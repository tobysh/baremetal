#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static int wait_until_reaped(int want, double limit) {
    double end = now() + limit;
    while (reaped < want && now() < end) {
        struct timespec ts = {0, 5 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    return reaped;
}

int main(void) {
    alarm(4);
    install_reaper();

    struct sigaction cur;
    sigaction(SIGCHLD, NULL, &cur);
    CHECK("SIGCHLD has a handler", cur.sa_handler != SIG_DFL && cur.sa_handler != SIG_IGN, "no handler installed");
    CHECK("SA_RESTART and SA_NOCLDSTOP are set", (cur.sa_flags & SA_RESTART) && (cur.sa_flags & SA_NOCLDSTOP), "flags = %d", cur.sa_flags);

    double t0 = now();
    char *const slow[] = {"sleep", "0.5", NULL};
    pid_t p = run_background(slow);
    double took = now() - t0;
    CHECK("run_background returns a pid at once", p > 0 && took < 0.25, "pid = %d, took %.2f s", (int)p, took);
    CHECK("the job is still running when we get the prompt back", kill(p, 0) == 0 && reaped == 0, "reaped = %d", (int)reaped);

    int n = wait_until_reaped(1, 2.0);
    CHECK("the finished job is reaped by the SIGCHLD handler", n == 1, "reaped = %d", n);
    errno = 0;
    CHECK("... and leaves no zombie", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still around");
    CHECK("... its pid is gone", kill(p, 0) == -1 && errno == ESRCH, "pid %d still exists", (int)p);

    int before = reaped;
    char *const quick[] = {"true", NULL};
    char *const fail[] = {"sh", "-c", "exit 3", NULL};
    char *const miss[] = {"no-such-command-bm", NULL};
    for (int i = 0; i < 4; i++) run_background(quick);
    run_background(fail);
    run_background(miss);
    n = wait_until_reaped(before + 6, 2.0) - before;
    CHECK("six jobs that end together are all reaped (the handler loops)", n == 6, "reaped %d of 6", n);
    errno = 0;
    CHECK("no zombies after the burst", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still around");

    before = reaped;
    for (int i = 0; i < 20; i++) {
        run_background(quick);
        struct timespec ts = {0, 2 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    wait_until_reaped(before + 20, 2.0);
    CHECK("twenty more jobs are all reaped", reaped - before == 20, "reaped %d of 20", (int)reaped - before);
    errno = 0;
    CHECK("... and none is left over", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still around");
    return 0;
}
