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

/* ---- public example: ready after 3 ticks, each read costs one tick ---- */
typedef struct { bus_t bus; uint32_t clock, ready_at; } demo;
static uint32_t d_read(bus_t *b, uint32_t off) {
    demo *d = (demo *)b; (void)off;
    uint32_t v = (int32_t)(d->clock - d->ready_at) >= 0 ? 1u : 0u;
    d->clock++;
    return v;
}
static void d_write(bus_t *b, uint32_t off, uint32_t v) { (void)b; (void)off; (void)v; }
static uint32_t d_now(bus_t *b) { return ((demo *)b)->clock; }

int main(void) {
    demo d = { { d_read, d_write, d_now }, 0xFFFFFFFEu, 0x00000001u };
    printf("ready within 10 ticks: %d (expect 0)\n", wait_flag(&d.bus, 0, 1, 10));
    demo e = { { d_read, d_write, d_now }, 100, 1000 };
    printf("never ready:           %d (expect -1)\n", wait_flag(&e.bus, 0, 1, 5));
    return 0;
}
