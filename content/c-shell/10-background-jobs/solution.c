#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t reaped = 0;

static void on_sigchld(int sig) {
    (void)sig;
    int saved = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) reaped++;
    errno = saved;
}

void install_reaper(void) {
    struct sigaction sa;
    sa.sa_handler = on_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

pid_t run_background(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    return pid;
}

int main(void) {
    install_reaper();
    char *const cmd[] = {"sleep", "0.2", NULL};
    pid_t pid = run_background(cmd);
    printf("started %d, prompt is back\n", (int)pid);
    sleep(1);
    printf("reaped %d\n", (int)reaped);
    return 0;
}
