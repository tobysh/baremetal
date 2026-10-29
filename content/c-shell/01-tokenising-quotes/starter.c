#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

int tokenize(const char *line, char *argv[], int max_args, char *buf, size_t buf_size) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    char *argv[16];
    char buf[256];
    int n = tokenize("grep \"hello world\" 'it is' a\\ b", argv, 15, buf, sizeof buf);
    printf("%d words\n", n);
    for (int i = 0; i < n; i++) printf("[%s]\n", argv[i]);
    return 0;
}
