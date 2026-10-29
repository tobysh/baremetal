#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t reaped = 0;

void install_reaper(void) {
}

pid_t run_background(char *const argv[]) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    install_reaper();
    char *const cmd[] = {"sleep", "0.2", NULL};
    pid_t pid = run_background(cmd);
    printf("started %d, prompt is back\n", (int)pid);
    sleep(1);
    printf("reaped %d\n", (int)reaped);
    return 0;
}
