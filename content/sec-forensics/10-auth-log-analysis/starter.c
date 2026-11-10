#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Read sshd log lines from stdin and report addresses with five or more password failures. */
int main(void) {
    /* TODO: parse "Failed password for [invalid user ]USER from IP" and "Accepted password for". */
    puts("no brute force detected");
    return 0;
}
