#include <stddef.h>
#include <stdint.h>

#define PIPE_CAP 16

/* The unread bytes are data[head], data[(head + 1) % PIPE_CAP], ...
   for count bytes. */
typedef struct {
    uint8_t data[PIPE_CAP];
    unsigned head;
    unsigned count;
    int closed;          /* 1 once the writer has closed its end */
} Pipe;

void pipe_init(Pipe *p) {
    p->head = 0;
    p->count = 0;
    p->closed = 0;
}

int pipe_write(Pipe *p, const void *src, size_t n) {
    if (p->closed)
        return -1;                      /* the writing end is gone */
    const uint8_t *s = src;
    size_t i = 0;
    while (i < n && p->count < PIPE_CAP) {
        p->data[(p->head + p->count) % PIPE_CAP] = s[i++];
        p->count++;
    }
    return (int)i;                      /* may be short, or 0 when full */
}

int pipe_read(Pipe *p, void *dst, size_t n) {
    if (n == 0)
        return 0;
    if (p->count == 0)
        return p->closed ? 0 : -1;      /* EOF versus "try again" */
    uint8_t *d = dst;
    size_t i = 0;
    while (i < n && p->count > 0) {
        d[i++] = p->data[p->head];
        p->head = (p->head + 1) % PIPE_CAP;
        p->count--;
    }
    return (int)i;
}

void pipe_close_writer(Pipe *p) {
    p->closed = 1;
}
