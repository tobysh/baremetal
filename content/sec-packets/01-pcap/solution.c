#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Read an unsigned integer of `bytes` bytes in the file's byte order. */
static uint32_t readnum(const uint8_t *p, int bytes, int big)
{
    uint32_t v = 0;
    if (big) {
        for (int i = 0; i < bytes; i++)
            v = (v << 8) | p[i];
    } else {
        for (int i = bytes - 1; i >= 0; i--)
            v = (v << 8) | p[i];
    }
    return v;
}

int pcap_count(const uint8_t *p, size_t n)
{
    if (!p || n < 24)
        return -1;

    /* The magic bytes decide the byte order for every other field. */
    int big;
    if (!memcmp(p, "\xd4\xc3\xb2\xa1", 4))
        big = 0;
    else if (!memcmp(p, "\xa1\xb2\xc3\xd4", 4))
        big = 1;
    else
        return -1;

    uint32_t snap = readnum(p + 16, 4, big);
    if (readnum(p + 4, 2, big) != 2 || readnum(p + 6, 2, big) != 4 || snap == 0 || readnum(p + 20, 4, big) != 1)
        return -1;

    size_t off = 24;
    int count = 0;
    while (off < n) {
        if (n - off < 16)
            return -1;                       /* partial record header */
        uint32_t micro = readnum(p + off + 4, 4, big);
        uint32_t cap = readnum(p + off + 8, 4, big);
        uint32_t orig = readnum(p + off + 12, 4, big);
        off += 16;
        if (micro >= 1000000 || cap > snap || cap > orig || cap > n - off)
            return -1;
        off += cap;
        count++;
    }
    return count;
}

int main(void)
{
    return 0;
}
