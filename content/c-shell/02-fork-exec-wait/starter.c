#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int run_command(char *const argv[]) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    char *const cmd[] = {"sh", "-c", "exit 5", NULL};
    printf("exit code %d\n", run_command(cmd));
    return 0;
}
