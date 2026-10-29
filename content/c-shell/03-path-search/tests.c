#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static void make(const char *path, const char *body, int mode) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd >= 0) { if (write(fd, body, strlen(body)) < 0) {} close(fd); }
    chmod(path, mode);
}

int main(void) {
    alarm(4);
    char out[256];
    char *base = mkdtemp((char[]){"/tmp/bm_pathXXXXXX"});
    char d1[300], d2[300], d3[300], p[900];
    snprintf(d1, sizeof d1, "%s/one", base);
    snprintf(d2, sizeof d2, "%s/two", base);
    snprintf(d3, sizeof d3, "%s/three", base);
    mkdir(d1, 0755); mkdir(d2, 0755); mkdir(d3, 0755);
    char f[400];
    snprintf(f, sizeof f, "%s/tool", d1); make(f, "#!/bin/sh\nexit 11\n", 0644);   /* not executable */
    snprintf(f, sizeof f, "%s/tool", d2); make(f, "#!/bin/sh\nexit 42\n", 0755);
    snprintf(f, sizeof f, "%s/tool", d3); make(f, "#!/bin/sh\nexit 43\n", 0755);
    snprintf(f, sizeof f, "%s/dirtool", d1); mkdir(f, 0755);
    snprintf(f, sizeof f, "%s/dirtool", d2); make(f, "#!/bin/sh\nexit 44\n", 0755);

    char want[400];
    snprintf(p, sizeof p, "/nonexistent:%s:%s", d1, d2);
    snprintf(want, sizeof want, "%s/tool", d2);
    int r = find_in_path("tool", p, out, sizeof out);
    CHECK("skips missing dirs and non-executable files", r == 0 && !strcmp(out, want), "r = %d, out = %s", r, out);

    snprintf(p, sizeof p, "%s:%s", d3, d2);
    snprintf(want, sizeof want, "%s/tool", d3);
    r = find_in_path("tool", p, out, sizeof out);
    CHECK("the first match wins", r == 0 && !strcmp(out, want), "r = %d, out = %s", r, out);

    snprintf(p, sizeof p, "%s:%s", d1, d2);
    snprintf(want, sizeof want, "%s/dirtool", d2);
    r = find_in_path("dirtool", p, out, sizeof out);
    CHECK("a directory with the right name does not count", r == 0 && !strcmp(out, want), "r = %d, out = %s", r, out);

    r = find_in_path("ls", "/nonexistent:/usr/bin", out, sizeof out);
    CHECK("finds ls in /usr/bin", r == 0 && !strcmp(out, "/usr/bin/ls"), "r = %d, out = %s", r, out);

    r = find_in_path("no-such-command-bm", "/usr/bin", out, sizeof out);
    CHECK("an unknown command returns -1", r == -1, "r = %d", r);

    r = find_in_path("ls", "", out, sizeof out);
    CHECK("an empty PATH finds nothing", r == -1, "r = %d", r);

    snprintf(p, sizeof p, "::%s::", d2);
    snprintf(want, sizeof want, "%s/tool", d2);
    r = find_in_path("tool", p, out, sizeof out);
    CHECK("empty entries are skipped", r == 0 && !strcmp(out, want), "r = %d, out = %s", r, out);

    r = find_in_path("/usr/bin/ls", "/nonexistent", out, sizeof out);
    CHECK("a name with a slash is used as it is", r == 0 && !strcmp(out, "/usr/bin/ls"), "r = %d, out = %s", r, out);
    r = find_in_path("/usr/bin/nope-bm", "/usr/bin", out, sizeof out);
    CHECK("a missing path with a slash fails", r == -1, "r = %d", r);

    char tiny[10];
    r = find_in_path("ls", "/usr/bin", tiny, sizeof tiny);
    CHECK("an output buffer that is too small fails", r == -1, "r = %d", r);

    char pcopy[] = "/nonexistent:/usr/bin";
    char orig[sizeof pcopy];
    strcpy(orig, pcopy);
    find_in_path("ls", pcopy, out, sizeof out);
    CHECK("path is not modified", !strcmp(pcopy, orig), "path = %s", pcopy);

    snprintf(p, sizeof p, "%s:%s", d1, d2);
    char *const run1[] = {"tool", NULL};
    r = run_with_path(run1, p);
    CHECK("run_with_path runs the first executable match", r == 42, "got %d", r);

    char *const run2[] = {"sh", "-c", "exit 9", NULL};
    r = run_with_path(run2, "/nonexistent:/usr/bin");
    CHECK("run_with_path passes the exit code on", r == 9, "got %d", r);

    char *const run3[] = {"no-such-command-bm", NULL};
    r = run_with_path(run3, "/usr/bin");
    CHECK("run_with_path gives 127 for an unknown command", r == 127, "got %d", r);

    char *const run4[] = {"sh", "-c", "[ \"$PATH\" = \"/usr/bin\" ] || exit 5; exit 0", NULL};
    r = run_with_path(run4, "/nonexistent");
    CHECK("it uses the given path, not the environment's", r == 127, "got %d (execvp would have found sh)", r);
    return 0;
}
