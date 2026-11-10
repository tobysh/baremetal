#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Read "time client server port outcome" lines from stdin and report scanning clients.
 * Count each (server, port) target once per client, using its first outcome.
 * Print "CLIENT targets=N open=O closed=C filtered=F" for each flagged client, or
 * "no scans detected" if there are none. See the task for the full rules. */
int main(void)
{
    char line[1024];
    while (fgets(line, sizeof line, stdin)) {
        /* TODO: parse the record, ignore invalid lines, and remember each client's targets. */
    }
    puts("no scans detected");
    return 0;
}
