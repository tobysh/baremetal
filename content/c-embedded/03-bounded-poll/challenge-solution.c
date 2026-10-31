#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);
    void     (*write)(struct bus *b, uint32_t off, uint32_t v);
    uint32_t (*now)(struct bus *b);
} bus_t;

// Wait until (reg & mask) == mask. 0 on success, -1 on timeout.
int wait_flag(bus_t *b, uint32_t off, uint32_t mask, uint32_t timeout) {
    uint32_t start = b->now(b);
    for (;;) {
        if ((b->read(b, off) & mask) == mask) return 0;
        if ((uint32_t)(b->now(b) - start) > timeout) return -1;
    }
}

static int poll_until(bus_t *b, uint32_t off, uint32_t mask, uint32_t start, uint32_t timeout) {
    for (;;) {
        if ((b->read(b, off) & mask) == mask) return 0;
        if ((uint32_t)(b->now(b) - start) > timeout) return -1;
    }
}

int wait_two(bus_t *b, uint32_t offA, uint32_t maskA, uint32_t offB, uint32_t maskB, uint32_t timeout) {
    uint32_t start = b->now(b);
    if (poll_until(b, offA, maskA, start, timeout) < 0) return -1;
    if (poll_until(b, offB, maskB, start, timeout) < 0) return -2;
    return 0;
}

int main(void) { return 0; }
