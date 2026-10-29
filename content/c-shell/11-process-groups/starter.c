#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t spawn_group(char *const argv[]) {
    return -1;
}

int kill_group(pid_t pgid, int sig) {
    return -1;
}

int wait_job(pid_t pid, int *stopped) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    char *const cmd[] = {"sleep", "30", NULL};
    pid_t job = spawn_group(cmd);
    printf("job %d, group %d, my group %d\n", (int)job, (int)getpgid(job), (int)getpgrp());
    kill_group(job, SIGTERM);
    int stopped = 0;
    printf("status %d\n", wait_job(job, &stopped));
    return 0;
}
