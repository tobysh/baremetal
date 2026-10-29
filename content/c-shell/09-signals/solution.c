#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t sigint_seen = 0;

static void on_sigint(int sig) {
    (void)sig;
    sigint_seen = 1;
}

void setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGQUIT, &sa, NULL);
}

int take_interrupt(void) {
    if (!sigint_seen) return 0;
    sigint_seen = 0;
    return 1;
}

int run_foreground(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        execvp(argv[0], argv);
        _exit(127);
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

int main(void) {
    setup_signals();
    char *const cmd[] = {"sh", "-c", "kill -INT $$", NULL};
    printf("status %d\n", run_foreground(cmd));
    raise(SIGINT);
    printf("shell survived SIGINT: %d\n", take_interrupt());
    return 0;
}
