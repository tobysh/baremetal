#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

int find_in_path(const char *name, const char *path, char *out, size_t out_size) {
    return -1;
}

int run_with_path(char *const argv[], const char *path) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    char buf[256];
    if (find_in_path("ls", "/nonexistent:/usr/bin", buf, sizeof buf) == 0) printf("found %s\n", buf);
    else printf("not found\n");
    return 0;
}
