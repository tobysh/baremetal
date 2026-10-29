#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static int is_number(const char *s) {
    if (*s == '-') s++;
    if (*s == '\0') return 0;
    for (; *s; s++)
        if (*s < '0' || *s > '9') return 0;
    return 1;
}

int run_builtin(char **argv, int *status) {
    int argc = 0;
    while (argv[argc]) argc++;
    if (strcmp(argv[0], "cd") == 0) {
        if (argc > 2) {
            fprintf(stderr, "cd: too many arguments\n");
            *status = 1;
            return 1;
        }
        const char *dir = argc == 2 ? argv[1] : getenv("HOME");
        if (!dir) {
            fprintf(stderr, "cd: HOME not set\n");
            *status = 1;
            return 1;
        }
        if (chdir(dir) != 0) {
            fprintf(stderr, "cd: %s: %s\n", dir, strerror(errno));
            *status = 1;
        } else {
            *status = 0;
        }
        return 1;
    }
    if (strcmp(argv[0], "exit") == 0) {
        if (argc > 2) {
            fprintf(stderr, "exit: too many arguments\n");
            *status = 1;
            return 1;
        }
        if (argc == 2) {
            if (!is_number(argv[1])) {
                fprintf(stderr, "exit: %s: numeric argument required\n", argv[1]);
                *status = 2;
                return 1;
            }
            *status = (int)(strtol(argv[1], NULL, 10) & 255);
        }
        return 2;
    }
    return 0;
}

int main(void) {
    int status = 0;
    char *cmd[] = {"cd", "/tmp", NULL};
    int r = run_builtin(cmd, &status);
    char buf[256];
    printf("result %d, status %d, cwd %s\n", r, status, getcwd(buf, sizeof buf));
    return 0;
}
