#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int run_redirect(char **argv) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    char *cmd[] = {"echo", "hello file", ">", "/tmp/out.txt", NULL};
    printf("status %d\n", run_redirect(cmd));
    char *show[] = {"cat", "/tmp/out.txt", NULL};
    run_redirect(show);
    return 0;
}
