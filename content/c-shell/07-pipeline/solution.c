#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static int wait_for(pid_t pid) {
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

int run_pipeline(char ***cmds, int n, int in_fd, int out_fd) {
    pid_t pids[n];
    int started = 0;
    int prev = in_fd;
    int own_prev = 0;   /* is prev a pipe end that we created? */
    int failed = 0;

    for (int i = 0; i < n; i++) {
        int p[2] = {-1, -1};
        if (i < n - 1 && pipe(p) < 0) { failed = 1; break; }
        pid_t pid = fork();
        if (pid < 0) {
            if (p[0] >= 0) { close(p[0]); close(p[1]); }
            failed = 1;
            break;
        }
        if (pid == 0) {
            if (prev >= 0) dup2(prev, STDIN_FILENO);
            if (p[1] >= 0) dup2(p[1], STDOUT_FILENO);
            else if (out_fd >= 0) dup2(out_fd, STDOUT_FILENO);
            if (prev > 2) close(prev);
            if (p[0] >= 0) close(p[0]);
            if (p[1] >= 0) close(p[1]);
            if (in_fd > 2) close(in_fd);
            if (out_fd > 2) close(out_fd);
            execvp(cmds[i][0], cmds[i]);
            _exit(127);
        }
        pids[started++] = pid;
        if (own_prev) close(prev);
        if (p[1] >= 0) close(p[1]);
        prev = p[0];
        own_prev = (p[0] >= 0);
    }
    if (own_prev) close(prev);

    int last = -1;
    for (int i = 0; i < started; i++) {
        int s = wait_for(pids[i]);
        if (i == n - 1) last = s;
    }
    return failed ? -1 : last;
}

int main(void) {
    char *a[] = {"printf", "b\\na\\nb\\n", NULL};
    char *b[] = {"sort", NULL};
    char *c[] = {"wc", "-l", NULL};
    char **cmds[] = {a, b, c};
    int r = run_pipeline(cmds, 3, -1, -1);
    printf("status %d\n", r);
    return 0;
}
