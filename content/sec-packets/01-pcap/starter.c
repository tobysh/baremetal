#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Count complete records in a classic pcap file, or return -1 if the file is
 * unsupported, malformed or truncated. See the task for every rule. */
int pcap_count(const uint8_t *p, size_t n)
{
    (void)p;
    (void)n;
    return -1;
}

int main(void)
{
    return 0;
}
