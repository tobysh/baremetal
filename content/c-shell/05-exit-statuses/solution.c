#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

enum { OP_SEQ, OP_AND, OP_OR };

static int run_wait(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    return status;
}

int exit_code(int wstatus) {
    if (wstatus == -1) return -1;
    if (WIFEXITED(wstatus)) return WEXITSTATUS(wstatus);
    if (WIFSIGNALED(wstatus)) return 128 + WTERMSIG(wstatus);
    if (WIFSTOPPED(wstatus)) return 128 + WSTOPSIG(wstatus);
    return -1;
}

int run_list(char *const a[], char *const b[], int op) {
    int sa = exit_code(run_wait(a));
    if (op == OP_AND && sa != 0) return sa;
    if (op == OP_OR && sa == 0) return sa;
    return exit_code(run_wait(b));
}

int main(void) {
    char *const f[] = {"false", NULL};
    char *const t[] = {"true", NULL};
    printf("false || true -> %d\n", run_list(f, t, OP_OR));
    return 0;
}
