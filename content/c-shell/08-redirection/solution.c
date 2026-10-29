#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static int is_op(const char *s) {
    return !strcmp(s, "<") || !strcmp(s, ">") || !strcmp(s, ">>") || !strcmp(s, "2>");
}

int run_redirect(char **argv) {
    int count = 0;
    for (; argv[count]; count++) {
        if (is_op(argv[count]) && argv[count + 1] == NULL) {
            fprintf(stderr, "sh: syntax error\n");
            return 2;
        }
        if (is_op(argv[count])) count++;
    }
    int words = 0;
    while (argv[words]) words++;

    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        char *clean[words + 1];
        int k = 0;
        for (int i = 0; argv[i]; i++) {
            if (!is_op(argv[i])) {
                clean[k++] = argv[i];
                continue;
            }
            const char *op = argv[i];
            const char *file = argv[++i];
            int fd, target;
            if (!strcmp(op, "<")) { fd = open(file, O_RDONLY); target = STDIN_FILENO; }
            else if (!strcmp(op, ">>")) { fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666); target = STDOUT_FILENO; }
            else { fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666); target = op[0] == '2' ? STDERR_FILENO : STDOUT_FILENO; }
            if (fd < 0) {
                fprintf(stderr, "sh: %s: %s\n", file, strerror(errno));
                _exit(1);
            }
            dup2(fd, target);
            close(fd);
        }
        clean[k] = NULL;
        if (k == 0) _exit(0);
        execvp(clean[0], clean);
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
    char *cmd[] = {"echo", "hello file", ">", "/tmp/out.txt", NULL};
    printf("status %d\n", run_redirect(cmd));
    char *show[] = {"cat", "/tmp/out.txt", NULL};
    run_redirect(show);
    return 0;
}
