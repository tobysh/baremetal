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

static int run_cap(char ***cmds, int n, const char *input, char *out, size_t sz, int *st) {
    int in = -1;
    if (input) {
        in = open("/tmp/bm_in.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (write(in, input, strlen(input)) < 0) {}
        lseek(in, 0, SEEK_SET);
    }
    int o = open("/tmp/bm_out.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
    *st = run_pipeline(cmds, n, in, o);
    lseek(o, 0, SEEK_SET);
    ssize_t got = read(o, out, sz - 1);
    out[got < 0 ? 0 : got] = '\0';
    close(o);
    if (in >= 0) close(in);
    return (int)got;
}

int main(void) {
    alarm(4);
    char out[8192];
    int st;

    char *c1[] = {"echo", "solo", NULL};
    char **one[] = {c1};
    run_cap(one, 1, NULL, out, sizeof out, &st);
    CHECK("a pipeline of one command", !strcmp(out, "solo\n") && st == 0, "out = %s, st = %d", out, st);

    char *a[] = {"cat", NULL};
    char *b[] = {"sort", NULL};
    char *c[] = {"tr", "a-z", "A-Z", NULL};
    char **three[] = {a, b, c};
    run_cap(three, 3, "pear\napple\nfig\n", out, sizeof out, &st);
    CHECK("cat | sort | tr with a file as input", !strcmp(out, "APPLE\nFIG\nPEAR\n") && st == 0, "out = %s, st = %d", out, st);

    char *d[] = {"printf", "b\\na\\nb\\na\\nc\\n", NULL};
    char *e[] = {"sort", NULL};
    char *f[] = {"tr", "-d", "\\n", NULL};
    char *g[] = {"wc", "-c", NULL};
    char **four[] = {d, e, f, g};
    run_cap(four, 4, NULL, out, sizeof out, &st);
    CHECK("four stages: count the characters", atoi(out) == 5 && st == 0, "out = %s, st = %d", out, st);

    char *h[] = {"cat", NULL};
    char *i2[] = {"cat", NULL};
    char *j[] = {"cat", NULL};
    char *k[] = {"cat", NULL};
    char *l[] = {"cat", NULL};
    char *m[] = {"cat", NULL};
    char **six[] = {h, i2, j, k, l, m};
    run_cap(six, 6, "through six pipes\n", out, sizeof out, &st);
    CHECK("six cats in a row pass the data through", !strcmp(out, "through six pipes\n") && st == 0, "out = %s, st = %d", out, st);

    /* more than a pipe buffer of data: needs all stages to run at once */
    char *y[] = {"sh", "-c", "i=0; while [ $i -lt 20000 ]; do echo line-$i; i=$((i+1)); done", NULL};
    char *w[] = {"wc", "-l", NULL};
    char **big[] = {y, a, w};
    run_cap(big, 3, NULL, out, sizeof out, &st);
    CHECK("a lot of data flows through (all stages run concurrently)", atoi(out) == 20000, "out = %s", out);

    char *fa[] = {"false", NULL};
    char *tr[] = {"true", NULL};
    char **ft[] = {fa, tr};
    run_cap(ft, 2, NULL, out, sizeof out, &st);
    CHECK("false | true is a success (last command counts)", st == 0, "st = %d", st);

    char **tf[] = {tr, fa};
    run_cap(tf, 2, NULL, out, sizeof out, &st);
    CHECK("true | false has status 1", st == 1, "st = %d", st);

    char *miss[] = {"no-such-command-bm", NULL};
    char **mid[] = {c1, miss, a};
    run_cap(mid, 3, NULL, out, sizeof out, &st);
    CHECK("a missing command in the middle does not hang", st == 0 && out[0] == '\0', "out = %s, st = %d", out, st);

    char **last[] = {c1, miss};
    run_cap(last, 2, NULL, out, sizeof out, &st);
    CHECK("a missing last command gives 127", st == 127, "st = %d", st);

    char *yes[] = {"sh", "-c", "while :; do echo y; done", NULL};
    char *hd[] = {"head", "-n", "3", NULL};
    char **yh[] = {yes, a, hd};
    run_cap(yh, 3, NULL, out, sizeof out, &st);
    CHECK("an endless producer is stopped by SIGPIPE", !strcmp(out, "y\ny\ny\n") && st == 0, "out = %s, st = %d", out, st);

    int before = open_fds();
    for (int r = 0; r < 8; r++) run_cap(four, 4, NULL, out, sizeof out, &st);
    CHECK("no file descriptors leak", open_fds() == before, "before %d, after %d", before, open_fds());

    errno = 0;
    CHECK("no zombie children are left behind", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still unreaped");

    int in = open("/tmp/bm_in2.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
    int o = open("/tmp/bm_out2.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
    run_pipeline(three, 3, in, o);
    CHECK("the caller's in_fd and out_fd stay open", fcntl(in, F_GETFD) != -1 && fcntl(o, F_GETFD) != -1, "a descriptor was closed");
    return 0;
}
