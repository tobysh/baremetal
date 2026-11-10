#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Escape in into out (at most cap bytes, zero terminated). Return the full escaped length. */
size_t log_escape(char *out, size_t cap, const char *in) {
    if (!in) in = "";
    /* TODO: \n, \r, \\ and \xHH for other control bytes; whole escapes only; return the full length. */
    if (out && cap) snprintf(out, cap, "%s", in);
    return strlen(in);
}

int main(void) { return 0; }
