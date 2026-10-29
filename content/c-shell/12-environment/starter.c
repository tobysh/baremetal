#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int expand_word(const char *in, char *out, size_t out_size, int last_status) {
    return -1;
}

int do_export(const char *assignment) {
    return 1;
}

int do_unset(const char *name) {
    return 1;
}

// Optional: try it with Run.
int main(void) {
    do_export("GREETING=hello");
    char out[128];
    int n = expand_word("[$GREETING] ${GREETING}world status=$? $", out, sizeof out, 3);
    printf("%d: %s\n", n, out);
    return 0;
}
