#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

static int usable(const char *p) {
    struct stat st;
    return stat(p, &st) == 0 && S_ISREG(st.st_mode) && access(p, X_OK) == 0;
}

int find_in_path(const char *name, const char *path, char *out, size_t out_size) {
    if (strchr(name, '/')) {
        if (strlen(name) >= out_size || !usable(name)) return -1;
        strcpy(out, name);
        return 0;
    }
    const char *p = path;
    for (;;) {
        const char *colon = strchr(p, ':');
        size_t len = colon ? (size_t)(colon - p) : strlen(p);
        if (len > 0) {
            int n = snprintf(out, out_size, "%.*s/%s", (int)len, p, name);
            if (n >= 0 && (size_t)n < out_size && usable(out)) return 0;
        }
        if (!colon) break;
        p = colon + 1;
    }
    return -1;
}

int run_with_path(char *const argv[], const char *path) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        char full[512];
        if (find_in_path(argv[0], path, full, sizeof full) == 0) execv(full, argv);
        _exit(127);
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int main(void) {
    char buf[256];
    if (find_in_path("ls", "/nonexistent:/usr/bin", buf, sizeof buf) == 0) printf("found %s\n", buf);
    else printf("not found\n");
    return 0;
}
