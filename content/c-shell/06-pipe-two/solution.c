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

int run_pipe2(char **left, char **right, int out_fd) {
    int p[2];
    if (pipe(p) < 0) return -1;

    pid_t a = fork();
    if (a < 0) {
        close(p[0]);
        close(p[1]);
        return -1;
    }
    if (a == 0) {
        dup2(p[1], STDOUT_FILENO);
        close(p[0]);
        close(p[1]);
        execvp(left[0], left);
        _exit(127);
    }

    pid_t b = fork();
    if (b < 0) {
        close(p[0]);
        close(p[1]);
        wait_for(a);
        return -1;
    }
    if (b == 0) {
        dup2(p[0], STDIN_FILENO);
        if (out_fd >= 0) {
            dup2(out_fd, STDOUT_FILENO);
            if (out_fd != STDOUT_FILENO) close(out_fd);
        }
        close(p[0]);
        close(p[1]);
        execvp(right[0], right);
        _exit(127);
    }

    close(p[0]);
    close(p[1]);
    wait_for(a);
    return wait_for(b);
}

int main(void) {
    char *left[] = {"echo", "hello pipes", NULL};
    char *right[] = {"tr", "a-z", "A-Z", NULL};
    int r = run_pipe2(left, right, -1);
    printf("status %d\n", r);
    return 0;
}
