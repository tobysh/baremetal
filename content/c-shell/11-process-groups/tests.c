#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static int group_gone(pid_t pgid) {
    for (int i = 0; i < 200; i++) {
        if (kill(-pgid, 0) == -1 && errno == ESRCH) return 1;
        struct timespec ts = {0, 10 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    return 0;
}

int main(void) {
    alarm(4);
    int stopped = -1;

    char *const quick[] = {"sh", "-c", "exit 5", NULL};
    pid_t q = spawn_group(quick);
    CHECK("the child leads a new group (checked immediately)", q > 0 && getpgid(q) == q && q != getpgrp(), "pid %d, group %d, mine %d", (int)q, (int)getpgid(q), (int)getpgrp());
    int r = wait_job(q, &stopped);
    CHECK("wait_job returns the exit code of a finished job", r == 5 && stopped == 0, "r = %d, stopped = %d", r, stopped);

    char *const missing[] = {"no-such-command-bm", NULL};
    pid_t m = spawn_group(missing);
    r = wait_job(m, &stopped);
    CHECK("an unknown command gives 127", r == 127 && stopped == 0, "r = %d", r);

    /* a job of three processes: a shell and two sleeps */
    char *const job[] = {"sh", "-c", "sleep 30 & sleep 30 & wait", NULL};
    pid_t j = spawn_group(job);
    CHECK("the job leads its own group", j > 0 && getpgid(j) == j, "group %d, pid %d", (int)getpgid(j), (int)j);
    struct timespec ts = {0, 100 * 1000 * 1000};
    nanosleep(&ts, NULL);   /* let the shell start its two sleeps */

    CHECK("kill_group returns 0 for a live group", kill_group(j, SIGSTOP) == 0, "kill_group failed");
    r = wait_job(j, &stopped);
    CHECK("a stopped job is reported with WUNTRACED", stopped == 1 && r == 128 + SIGSTOP, "r = %d, stopped = %d", r, stopped);

    kill_group(j, SIGCONT);
    kill_group(j, SIGTERM);
    r = wait_job(j, &stopped);
    CHECK("after SIGTERM the job reports 128 + 15", stopped == 0 && r == 128 + SIGTERM, "r = %d, stopped = %d", r, stopped);
    CHECK("the whole group is gone, not just its leader", group_gone(j), "some member of group %d is still alive", (int)j);

    CHECK("kill_group fails for a group that does not exist", kill_group(j, SIGTERM) == -1, "it reported success");

    r = wait_job(j, &stopped);
    CHECK("wait_job fails for a child that is already reaped", r == -1, "r = %d", r);

    CHECK("this process is still in its original group", getpgid(0) == getpgrp(), "group changed");
    errno = 0;
    CHECK("no zombie children are left behind", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still unreaped");
    return 0;
}
