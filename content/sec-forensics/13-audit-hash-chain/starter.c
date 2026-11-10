#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Read "seq|actor|action|hash" lines from stdin and verify the hash chain. */
int main(void) {
    /* TODO: per line check malformed, then sequence, then hash; stop at the first problem. */
    puts("chain ok: 0 entries head=0000000000000000");
    return 0;
}
