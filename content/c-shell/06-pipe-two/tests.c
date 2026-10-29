#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static int open_fds(void) {
    int n = 0;
    DIR *d = opendir("/proc/self/fd");
    if (!d) return -1;
    while (readdir(d)) n++;
    closedir(d);
    return n;
}

/* Runs left | right with the output captured in a temp file. */
static int capture(char **l, char **r, char *out, size_t n, int *status) {
    int fd = open("/tmp/bm_cap.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
    *status = run_pipe2(l, r, fd);
    lseek(fd, 0, SEEK_SET);
    ssize_t got = read(fd, out, n - 1);
    out[got < 0 ? 0 : got] = '\0';
    close(fd);
    return (int)got;
}

int main(void) {
    alarm(4);
    char out[4096];
    int st;

    char *l1[] = {"echo", "hello", NULL};
    char *r1[] = {"tr", "a-z", "A-Z", NULL};
    capture(l1, r1, out, sizeof out, &st);
    CHECK("echo hello | tr a-z A-Z", !strcmp(out, "HELLO\n") && st == 0, "out = %s, st = %d", out, st);

    char *l2[] = {"printf", "b\\nc\\na\\n", NULL};
    char *r2[] = {"sort", NULL};
    capture(l2, r2, out, sizeof out, &st);
    CHECK("printf | sort sorts the lines", !strcmp(out, "a\nb\nc\n"), "out = %s", out);

    char *l3[] = {"printf", "one\\ntwo\\nthree\\n", NULL};
    char *r3[] = {"wc", "-l", NULL};
    capture(l3, r3, out, sizeof out, &st);
    int lines = atoi(out);
    CHECK("the reader sees end-of-file (wc -l counts 3)", lines == 3, "out = %s", out);

    char *l4[] = {"true", NULL};
    char *r4[] = {"sh", "-c", "exit 3", NULL};
    capture(l4, r4, out, sizeof out, &st);
    CHECK("returns the status of the right-hand command", st == 3, "st = %d", st);

    char *l5[] = {"sh", "-c", "exit 9", NULL};
    char *r5[] = {"cat", NULL};
    capture(l5, r5, out, sizeof out, &st);
    CHECK("the left command's status is ignored", st == 0 && out[0] == '\0', "st = %d", st);

    char *l6[] = {"no-such-command-bm", NULL};
    capture(l6, r5, out, sizeof out, &st);
    CHECK("a missing left command gives the reader an empty input", st == 0 && out[0] == '\0', "st = %d", st);

    char *r7[] = {"no-such-command-bm", NULL};
    capture(l1, r7, out, sizeof out, &st);
    CHECK("a missing right command gives 127", st == 127, "st = %d", st);

    char *r8[] = {"sh", "-c", "kill -9 $$", NULL};
    capture(l1, r8, out, sizeof out, &st);
    CHECK("a right command killed by a signal gives 128 + signal", st == 137, "st = %d", st);

    char *l9[] = {"sh", "-c", "while :; do echo x; done", NULL};
    char *r9[] = {"head", "-n", "2", NULL};
    capture(l9, r9, out, sizeof out, &st);
    CHECK("an endless writer stops when the reader quits (SIGPIPE)", !strcmp(out, "x\nx\n") && st == 0, "out = %s, st = %d", out, st);

    int before = open_fds();
    for (int i = 0; i < 10; i++) capture(l1, r1, out, sizeof out, &st);
    CHECK("no file descriptors leak", open_fds() == before, "fds before %d, after %d", before, open_fds());

    errno = 0;
    CHECK("no zombie children are left behind", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still unreaped");

    int fd = open("/tmp/bm_cap2.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
    run_pipe2(l1, r1, fd);
    CHECK("out_fd is still open for the caller", fcntl(fd, F_GETFD) != -1, "out_fd was closed");
    close(fd);
    return 0;
}
