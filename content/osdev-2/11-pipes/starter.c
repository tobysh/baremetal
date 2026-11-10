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
    /* TODO */
    (void)p;
}

int pipe_write(Pipe *p, const void *src, size_t n) {
    /* TODO: -1 after close; otherwise append what fits, return the count */
    (void)p;
    (void)src;
    (void)n;
    return -1;
}

int pipe_read(Pipe *p, void *dst, size_t n) {
    /* TODO: 0 for n == 0; empty -> -1 (open) or 0 (EOF); else drain up to n */
    (void)p;
    (void)dst;
    (void)n;
    return -1;
}

void pipe_close_writer(Pipe *p) {
    /* TODO */
    (void)p;
}
