#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int run_pipeline(char ***cmds, int n, int in_fd, int out_fd) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    char *a[] = {"printf", "b\\na\\nb\\n", NULL};
    char *b[] = {"sort", NULL};
    char *c[] = {"wc", "-l", NULL};
    char **cmds[] = {a, b, c};
    int r = run_pipeline(cmds, 3, -1, -1);
    printf("status %d\n", r);
    return 0;
}
