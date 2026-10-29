#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

int tokenize(const char *line, char *argv[], int max_args, char *buf, size_t buf_size) {
    size_t o = 0;
    int n = 0;
    const char *p = line;
#define PUT(c) do { if (o >= buf_size) return -2; buf[o++] = (c); } while (0)
    for (;;) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        if (n >= max_args) return -2;
        argv[n] = buf + o;
        while (*p && *p != ' ' && *p != '\t') {
            if (*p == '\'') {
                p++;
                while (*p && *p != '\'') PUT(*p++);
                if (*p == '\0') return -1;
                p++;
            } else if (*p == '"') {
                p++;
                while (*p && *p != '"') {
                    if (*p == '\\' && (p[1] == '"' || p[1] == '\\')) p++;
                    PUT(*p++);
                }
                if (*p == '\0') return -1;
                p++;
            } else {
                if (*p == '\\' && p[1]) p++;
                PUT(*p++);
            }
        }
        PUT('\0');
        n++;
    }
#undef PUT
    argv[n] = NULL;
    return n;
}

int main(void) {
    char *argv[16];
    char buf[256];
    int n = tokenize("grep \"hello world\" 'it is' a\\ b", argv, 15, buf, sizeof buf);
    printf("%d words\n", n);
    for (int i = 0; i < n; i++) printf("[%s]\n", argv[i]);
    return 0;
}
