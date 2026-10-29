#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int run_builtin(char **argv, int *status) {
    return 0;
}

// Optional: try it with Run.
int main(void) {
    int status = 0;
    char *cmd[] = {"cd", "/tmp", NULL};
    int r = run_builtin(cmd, &status);
    char buf[256];
    printf("result %d, status %d, cwd %s\n", r, status, getcwd(buf, sizeof buf));
    return 0;
}
