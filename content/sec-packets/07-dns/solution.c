#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((unsigned)p[0] << 8 | p[1]);
}

typedef struct {
    uint16_t id, type, qclass;
    char name[254];          /* dotted display name, NUL-terminated; "." for the root */
} Question;

static int label_char(unsigned c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-';
}

int dns_question(const uint8_t *p, size_t n, Question *out)
{
    if (!p || !out || n < 12 || be16(p + 4) != 1)
        return -1;

    Question q = {0};
    q.id = be16(p);
    size_t off = 12;   /* next unread byte of the message */
    size_t k = 0;      /* characters written to q.name */

    for (;;) {
        if (off >= n)
            return -1;                       /* name never ended */
        size_t len = p[off++];
        if (len == 0)
            break;
        if (len > 63 || len > n - off)
            return -1;                       /* pointer, reserved bits, or label past the end */
        if (k) {
            if (k >= 253)
                return -1;
            q.name[k++] = '.';
        }
        if (len > 253 - k)
            return -1;                       /* display name would exceed 253 characters */
        for (size_t i = 0; i < len; i++) {
            if (!label_char(p[off + i]))
                return -1;
            q.name[k++] = (char)p[off + i];
        }
        off += len;
    }
    if (k == 0)
        q.name[k++] = '.';
    q.name[k] = 0;

    if (n - off < 4)
        return -1;                           /* QTYPE and QCLASS are missing */
    q.type = be16(p + off);
    q.qclass = be16(p + off + 2);
    *out = q;
    return 0;
}

int main(void)
{
    return 0;
}
