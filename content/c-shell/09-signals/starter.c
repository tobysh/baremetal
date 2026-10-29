#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t sigint_seen = 0;

void setup_signals(void) {
}

int take_interrupt(void) {
    return 0;
}

int run_foreground(char *const argv[]) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    setup_signals();
    char *const cmd[] = {"sh", "-c", "kill -INT $$", NULL};
    printf("status %d\n", run_foreground(cmd));
    raise(SIGINT);
    printf("shell survived SIGINT: %d\n", take_interrupt());
    return 0;
}
