#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Read "UnixSeconds source message" lines from stdin, sort stably and print the timeline. */
int main(void) {
    /* TODO: parse with fgets + sscanf("%lld %63s %n"), stable sort, gmtime/strftime, quiet gaps. */
    puts("0 events over 0h 00m");
    return 0;
}
