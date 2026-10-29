#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    /* Helper mode: this program re-executes itself as the "command" and raises a signal. */
    if (argc > 2 && !strcmp(argv[1], "raise")) {
        raise(atoi(argv[2]));
        usleep(200000);
        _exit(0);
    }
    alarm(4);
    CHECK("take_interrupt is 0 before any signal", take_interrupt() == 0, "flag already set");

    setup_signals();

    struct sigaction cur;
    sigaction(SIGINT, NULL, &cur);
    CHECK("SIGINT now has a handler", cur.sa_handler != SIG_DFL && cur.sa_handler != SIG_IGN, "handler is default or ignore");
    CHECK("SIGINT is installed with SA_RESTART", (cur.sa_flags & SA_RESTART) != 0, "flags = %d", cur.sa_flags);
    sigaction(SIGQUIT, NULL, &cur);
    CHECK("SIGQUIT is ignored", cur.sa_handler == SIG_IGN, "SIGQUIT is not ignored");

    raise(SIGINT);
    CHECK("the shell survives SIGINT and sets the flag", sigint_seen == 1, "sigint_seen = %d", (int)sigint_seen);
    CHECK("take_interrupt returns 1 once", take_interrupt() == 1, "got 0");
    CHECK("... and then clears the flag", take_interrupt() == 0 && sigint_seen == 0, "flag still set");

    raise(SIGQUIT);
    CHECK("the shell survives SIGQUIT", 1, "");

    char *const ok[] = {"sh", "-c", "exit 4", NULL};
    CHECK("run_foreground returns the exit code", run_foreground(ok) == 4, "got %d", run_foreground(ok));

    char *const intr[] = {"/proc/self/exe", "raise", "2", NULL};
    int r = run_foreground(intr);
    CHECK("a child killed by SIGINT gives 130", r == 130, "got %d", r);
    CHECK("... and the shell's own flag is not set by it", sigint_seen == 0, "flag set");

    char *const quit[] = {"/proc/self/exe", "raise", "3", NULL};
    r = run_foreground(quit);
    CHECK("a child killed by SIGQUIT gives 131 (default restored before exec)", r == 131, "got %d", r);

    char *const term[] = {"sh", "-c", "kill -TERM $$", NULL};
    CHECK("SIGTERM gives 143", run_foreground(term) == 143, "got %d", run_foreground(term));

    char *const miss[] = {"no-such-command-bm", NULL};
    CHECK("an unknown command gives 127", run_foreground(miss) == 127, "got %d", run_foreground(miss));

    /* the shell keeps its own settings after running children */
    sigaction(SIGQUIT, NULL, &cur);
    int quit_ign = cur.sa_handler == SIG_IGN;
    sigaction(SIGINT, NULL, &cur);
    CHECK("the shell's own settings are unchanged after running children", quit_ign && cur.sa_handler != SIG_DFL && cur.sa_handler != SIG_IGN, "settings changed");

    /* a signal arriving while we wait must not break waitpid */
    pid_t me = getpid();
    pid_t helper = fork();
    if (helper == 0) {
        usleep(100000);
        kill(me, SIGINT);
        _exit(0);
    }
    char *const slow[] = {"sh", "-c", "sleep 0.4; exit 6", NULL};
    r = run_foreground(slow);
    waitpid(helper, NULL, 0);
    CHECK("a SIGINT while waiting does not lose the child", r == 6 && sigint_seen == 1, "r = %d, flag = %d", r, (int)sigint_seen);

    errno = 0;
    CHECK("no zombie children are left behind", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still unreaped");
    return 0;
}
