#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Walk record headers in b[0..n). See the task for the return codes (0, 1, -1) and the output counts.
 * This starter trusts every header, never reports "wait" or "invalid", and can read past the end. */
int tls_walk(const uint8_t *b, size_t n, size_t *records, size_t *consumed)
{
    size_t pos = 0, rc = 0;

    while (pos + 5 <= n) {
        size_t l = ((size_t)b[pos + 3] << 8) | b[pos + 4];
        pos += 5 + l;
        rc++;
    }
    if (records)
        *records = rc;
    if (consumed)
        *consumed = pos;
    return 0;
}

int main(void)
{
    return 0;
}
