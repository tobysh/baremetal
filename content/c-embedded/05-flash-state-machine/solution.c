#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);
    void     (*write)(struct bus *b, uint32_t off, uint32_t v);
    uint32_t (*now)(struct bus *b);
} bus_t;

enum { CMD = 0x00, ADDR = 0x04, DATA = 0x08, STATUS = 0x0C, KEY = 0x10 };
enum { BUSY = 1u << 0, ERR = 1u << 1, LOCKED = 1u << 2 };
enum { CMD_PROGRAM = 1 };
#define KEY1 0x45670123u
#define KEY2 0xCDEF89ABu

// Provided: 0 when BUSY is clear, -1 if still busy after `timeout` ticks.
static int wait_not_busy(bus_t *b, uint32_t timeout) {
    uint32_t start = b->now(b);
    for (;;) {
        if (!(b->read(b, STATUS) & BUSY)) return 0;
        if ((uint32_t)(b->now(b) - start) > timeout) return -1;
    }
}

int flash_unlock(bus_t *b) {
    if (b->read(b, STATUS) & LOCKED) {
        b->write(b, KEY, KEY1);
        b->write(b, KEY, KEY2);
    }
    return (b->read(b, STATUS) & LOCKED) ? -4 : 0;
}

int flash_program(bus_t *b, uint32_t addr, uint32_t data, uint32_t timeout) {
    if (flash_unlock(b) != 0) return -4;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (wait_not_busy(b, timeout) < 0) return -1;
        b->write(b, ADDR, addr);
        b->write(b, DATA, data);
        b->write(b, CMD, CMD_PROGRAM);
        if (wait_not_busy(b, timeout) < 0) return -1;
        if (!(b->read(b, STATUS) & ERR)) return 0;
        b->write(b, STATUS, ERR);
    }
    return -3;
}

/* ---- public example: a controller that never fails and finishes at once ---- */
typedef struct { bus_t bus; uint32_t locked, word; } demo;
static uint32_t d_read(bus_t *b, uint32_t off) { demo *d = (demo *)b; return off == STATUS && d->locked ? LOCKED : 0; }
static void d_write(bus_t *b, uint32_t off, uint32_t v) {
    demo *d = (demo *)b; static int k;
    if (off == KEY) { if (v == KEY1) k = 1; else if (v == KEY2 && k == 1) d->locked = 0; }
    if (off == DATA) d->word = v;
}
static uint32_t d_now(bus_t *b) { (void)b; return 0; }

int main(void) {
    demo d = { { d_read, d_write, d_now }, 1, 0 };
    printf("program: %d (expect 0)\n", flash_program(&d.bus, 0x10, 0xCAFEBABEu, 5));
    printf("word 0x%08X (expect 0xCAFEBABE), unlock again: %d (expect 0)\n", d.word, flash_unlock(&d.bus));
    return 0;
}
