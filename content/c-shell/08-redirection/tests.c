#include <sys/stat.h>
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

static void slurp(const char *path, char *out, size_t n) {
    int fd = open(path, O_RDONLY);
    ssize_t got = fd < 0 ? -1 : read(fd, out, n - 1);
    if (fd >= 0) close(fd);
    if (got < 0) { strcpy(out, "<missing>"); return; }
    out[got] = '\0';
}

static void put(const char *path, const char *text) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (write(fd, text, strlen(text)) < 0) {}
    close(fd);
}

int main(void) {
    alarm(4);
    umask(022);
    char got[256];
    int r;

    unlink("/tmp/bm_o1");
    char *c1[] = {"echo", "hello", "world", ">", "/tmp/bm_o1", NULL};
    r = run_redirect(c1);
    slurp("/tmp/bm_o1", got, sizeof got);
    CHECK("echo hello world > file", r == 0 && !strcmp(got, "hello world\n"), "r = %d, file = %s", r, got);

    put("/tmp/bm_o2", "old contents that are long\n");
    char *c2[] = {"echo", "new", ">", "/tmp/bm_o2", NULL};
    run_redirect(c2);
    slurp("/tmp/bm_o2", got, sizeof got);
    CHECK("> truncates an existing file", !strcmp(got, "new\n"), "file = %s", got);

    char *c3[] = {"echo", "more", ">>", "/tmp/bm_o2", NULL};
    run_redirect(c3);
    slurp("/tmp/bm_o2", got, sizeof got);
    CHECK(">> appends", !strcmp(got, "new\nmore\n"), "file = %s", got);

    put("/tmp/bm_i1", "pear\napple\nfig\n");
    char *c4[] = {"sort", "<", "/tmp/bm_i1", ">", "/tmp/bm_o4", NULL};
    r = run_redirect(c4);
    slurp("/tmp/bm_o4", got, sizeof got);
    CHECK("sort < in > out", r == 0 && !strcmp(got, "apple\nfig\npear\n"), "r = %d, file = %s", r, got);

    char *c5[] = {">", "/tmp/bm_o5", "echo", "front", NULL};
    run_redirect(c5);
    slurp("/tmp/bm_o5", got, sizeof got);
    CHECK("the redirection may come first", !strcmp(got, "front\n"), "file = %s", got);

    char *c6[] = {"sh", "-c", "echo to-out; echo to-err >&2", "2>", "/tmp/bm_e6", ">", "/tmp/bm_o6", NULL};
    run_redirect(c6);
    char e6[256];
    slurp("/tmp/bm_o6", got, sizeof got);
    slurp("/tmp/bm_e6", e6, sizeof e6);
    CHECK("2> redirects stderr separately", !strcmp(got, "to-out\n") && !strcmp(e6, "to-err\n"), "out = %s, err = %s", got, e6);

    char *c7[] = {"echo", "x", ">", "/tmp/bm_o7a", ">", "/tmp/bm_o7b", NULL};
    run_redirect(c7);
    char a7[64], b7[64];
    slurp("/tmp/bm_o7a", a7, sizeof a7);
    slurp("/tmp/bm_o7b", b7, sizeof b7);
    CHECK("with two > the last one wins; both files exist", !strcmp(a7, "") && !strcmp(b7, "x\n"), "a = %s, b = %s", a7, b7);

    char *c8[] = {"cat", "<", "/tmp/bm_does_not_exist", ">", "/tmp/bm_o8", NULL};
    unlink("/tmp/bm_o8");
    r = run_redirect(c8);
    CHECK("a missing input file gives status 1", r == 1, "r = %d", r);

    char *c9[] = {"echo", "hi", ">", "/no/such/dir/file", NULL};
    r = run_redirect(c9);
    CHECK("an unwritable output file gives status 1", r == 1, "r = %d", r);

    unlink("/tmp/bm_o10");
    char *c10[] = {"touch", "/tmp/bm_ran10", "<", "/tmp/bm_nope", NULL};
    unlink("/tmp/bm_ran10");
    run_redirect(c10);
    CHECK("the command is not run when a redirection fails", access("/tmp/bm_ran10", F_OK) != 0, "the command ran anyway");

    char *c11[] = {"echo", "hi", ">", NULL};
    r = run_redirect(c11);
    CHECK("an operator with no filename gives status 2", r == 2, "r = %d", r);

    char *c12[] = {"sh", "-c", "exit 6", ">", "/tmp/bm_o12", NULL};
    r = run_redirect(c12);
    CHECK("the command's exit code is returned", r == 6, "r = %d", r);

    char *c13[] = {"no-such-command-bm", ">", "/tmp/bm_o13", NULL};
    r = run_redirect(c13);
    CHECK("an unknown command gives 127", r == 127, "r = %d", r);

    char *c14[] = {">", "/tmp/bm_o14", NULL};
    r = run_redirect(c14);
    CHECK("only a redirection and no command succeeds", r == 0, "r = %d", r);

    put("/tmp/bm_i15", "keep\n");
    char *c16[] = {"echo", "a", ">", "/tmp/bm_o16", NULL};
    char before[8];
    snprintf(before, sizeof before, "%d", open_fds());
    for (int i = 0; i < 10; i++) { run_redirect(c16); }
    CHECK("no descriptors leak and the parent's stdout is untouched", open_fds() == atoi(before) && write(1, "", 0) == 0, "fds before %s, after %d", before, open_fds());

    struct stat st;
    unlink("/tmp/bm_o17");
    char *c17[] = {"echo", "x", ">", "/tmp/bm_o17", NULL};
    run_redirect(c17);
    stat("/tmp/bm_o17", &st);
    CHECK("new files get mode 0666 minus the umask", (st.st_mode & 0777) == 0644, "mode = %o", st.st_mode & 0777);

    errno = 0;
    CHECK("no zombie children are left behind", waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD, "a child is still unreaped");
    return 0;
}
