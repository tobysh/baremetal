#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static char cwd[512];
static const char *here(void) { return getcwd(cwd, sizeof cwd); }

int main(void) {
    alarm(4);
    int st = 77;
    int r;

    char *notb[] = {"ls", "-l", NULL};
    r = run_builtin(notb, &st);
    CHECK("an ordinary command is not a builtin", r == 0 && st == 77, "r = %d, st = %d", r, st);

    char *cd1[] = {"cd", "/usr", NULL};
    r = run_builtin(cd1, &st);
    CHECK("cd /usr changes directory", r == 1 && st == 0 && !strcmp(here(), "/usr"), "r = %d, st = %d, cwd = %s", r, st, here());

    char *cd2[] = {"cd", "/usr/bin", NULL};
    run_builtin(cd2, &st);
    char *cd3[] = {"cd", "..", NULL};
    r = run_builtin(cd3, &st);
    CHECK("cd .. goes up one level", r == 1 && st == 0 && !strcmp(here(), "/usr"), "cwd = %s", here());

    st = 0;
    char *cd4[] = {"cd", "/no/such/dir-bm", NULL};
    r = run_builtin(cd4, &st);
    CHECK("cd to a missing directory sets status 1 and stays put", r == 1 && st == 1 && !strcmp(here(), "/usr"), "r = %d, st = %d, cwd = %s", r, st, here());

    setenv("HOME", "/tmp", 1);
    st = 5;
    char *cd5[] = {"cd", NULL};
    r = run_builtin(cd5, &st);
    CHECK("bare cd goes to $HOME", r == 1 && st == 0 && !strcmp(here(), "/tmp"), "r = %d, st = %d, cwd = %s", r, st, here());

    unsetenv("HOME");
    char *cd6[] = {"cd", "/usr", NULL};
    run_builtin(cd6, &st);
    r = run_builtin(cd5, &st);
    CHECK("bare cd without $HOME fails with status 1", r == 1 && st == 1 && !strcmp(here(), "/usr"), "r = %d, st = %d, cwd = %s", r, st, here());

    st = 0;
    char *cd7[] = {"cd", "/tmp", "/usr", NULL};
    r = run_builtin(cd7, &st);
    CHECK("cd with two arguments is an error", r == 1 && st == 1 && !strcmp(here(), "/usr"), "r = %d, st = %d, cwd = %s", r, st, here());

    st = 9;
    char *ex1[] = {"exit", NULL};
    r = run_builtin(ex1, &st);
    CHECK("bare exit asks to quit and keeps the status", r == 2 && st == 9, "r = %d, st = %d", r, st);

    char *ex2[] = {"exit", "3", NULL};
    r = run_builtin(ex2, &st);
    CHECK("exit 3 sets the status to 3", r == 2 && st == 3, "r = %d, st = %d", r, st);

    char *ex3[] = {"exit", "300", NULL};
    r = run_builtin(ex3, &st);
    CHECK("exit 300 wraps to 44", r == 2 && st == 44, "r = %d, st = %d", r, st);

    char *ex4[] = {"exit", "-1", NULL};
    r = run_builtin(ex4, &st);
    CHECK("exit -1 wraps to 255", r == 2 && st == 255, "r = %d, st = %d", r, st);

    st = 0;
    char *ex5[] = {"exit", "abc", NULL};
    r = run_builtin(ex5, &st);
    CHECK("exit abc does not quit and sets status 2", r == 1 && st == 2, "r = %d, st = %d", r, st);

    st = 0;
    char *ex6[] = {"exit", "1", "2", NULL};
    r = run_builtin(ex6, &st);
    CHECK("exit with two arguments does not quit", r == 1 && st == 1, "r = %d, st = %d", r, st);

    char *ex7[] = {"exits", NULL};
    char *ex8[] = {"cdx", "/tmp", NULL};
    st = 4;
    CHECK("names must match exactly", run_builtin(ex7, &st) == 0 && run_builtin(ex8, &st) == 0 && st == 4, "st = %d", st);

    /* messages go to stderr, not stdout */
    int saved = dup(2);
    int fd = open("/tmp/bm_err.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
    dup2(fd, 2);
    char *bad[] = {"cd", "/no/such/dir-bm", NULL};
    run_builtin(bad, &st);
    dup2(saved, 2);
    close(saved);
    char msg[256] = {0};
    lseek(fd, 0, SEEK_SET);
    ssize_t got = read(fd, msg, sizeof msg - 1);
    close(fd);
    CHECK("cd failure message goes to stderr", got > 0 && strstr(msg, "cd: /no/such/dir-bm: No such file or directory") != NULL, "stderr was: %s", msg);
    return 0;
}
