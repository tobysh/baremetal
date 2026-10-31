#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);
    void     (*write)(struct bus *b, uint32_t off, uint32_t v);
    uint32_t (*now)(struct bus *b);
} bus_t;

enum { CTRL = 0x00, STATUS = 0x04, DATA = 0x08 };
enum { START = 1u << 0, DONE = 1u << 0, FAULT = 1u << 1 };

// 0 ok, -1 timeout, -2 sensor fault.
int sensor_read_mC(bus_t *b, int32_t *out, uint32_t timeout) {
    uint32_t start = b->now(b);
    b->write(b, CTRL, START);
    for (;;) {
        uint32_t st = b->read(b, STATUS);
        if (st & FAULT) { b->write(b, STATUS, FAULT); return -2; }
        if (st & DONE) break;
        if ((uint32_t)(b->now(b) - start) > timeout) return -1;
    }
    int32_t raw = (int32_t)((b->read(b, DATA) & 0xFFFu) ^ 0x800u) - 0x800;
    int32_t mag = raw < 0 ? -raw : raw;
    int32_t mc = (mag * 125 + 1) / 2;
    *out = raw < 0 ? -mc : mc;
    return 0;
}

/* ---- public example: a sensor that always reads 25.0 C after 2 ticks ---- */
typedef struct { bus_t bus; uint32_t clock, done_at; int started; } demo;
static uint32_t d_read(bus_t *b, uint32_t off) {
    demo *d = (demo *)b;
    if (off == STATUS) { uint32_t v = d->started && d->clock >= d->done_at ? DONE : 0; d->clock++; return v; }
    d->started = 0;
    return 0xA5A5A000u | 0x190u;
}
static void d_write(bus_t *b, uint32_t off, uint32_t v) {
    demo *d = (demo *)b;
    if (off == CTRL && (v & START)) { d->started = 1; d->done_at = d->clock + 2; }
}
static uint32_t d_now(bus_t *b) { return ((demo *)b)->clock; }

int main(void) {
    demo d = { { d_read, d_write, d_now }, 0, 0, 0 };
    int32_t mc = 0;
    int r = sensor_read_mC(&d.bus, &mc, 10);
    printf("status %d, %d mC (expect 0, 25000)\n", r, mc);
    return 0;
}
