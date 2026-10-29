#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t spawn_group(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        setpgid(0, 0);
        execvp(argv[0], argv);
        _exit(127);
    }
    setpgid(pid, pid);
    return pid;
}

int kill_group(pid_t pgid, int sig) {
    return kill(-pgid, sig) == 0 ? 0 : -1;
}

int wait_job(pid_t pid, int *stopped) {
    int status;
    *stopped = 0;
    while (waitpid(pid, &status, WUNTRACED) < 0) {
        if (errno != EINTR) return -1;
    }
    if (WIFSTOPPED(status)) {
        *stopped = 1;
        return 128 + WSTOPSIG(status);
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

int main(void) {
    char *const cmd[] = {"sleep", "30", NULL};
    pid_t job = spawn_group(cmd);
    printf("job %d, group %d, my group %d\n", (int)job, (int)getpgid(job), (int)getpgrp());
    kill_group(job, SIGTERM);
    int stopped = 0;
    printf("status %d\n", wait_job(job, &stopped));
    return 0;
}
