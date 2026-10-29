#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int run_pipe2(char **left, char **right, int out_fd) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    char *left[] = {"echo", "hello pipes", NULL};
    char *right[] = {"tr", "a-z", "A-Z", NULL};
    int r = run_pipe2(left, right, -1);
    printf("status %d\n", r);
    return 0;
}
