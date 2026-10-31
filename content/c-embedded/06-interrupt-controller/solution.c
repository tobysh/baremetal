#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);
    void     (*write)(struct bus *b, uint32_t off, uint32_t v);
    uint32_t (*now)(struct bus *b);
} bus_t;

enum { ENABLE = 0x00, PENDING = 0x04, ACK = 0x08 };
#define NLINES 8
#define MAX_PASSES 16

typedef void (*isr_t)(void *ctx);
typedef struct { isr_t isr[NLINES]; void *ctx[NLINES]; uint32_t spurious; } irq_table_t;

int irq_register(irq_table_t *t, unsigned line, isr_t isr, void *ctx) {
    if (line >= NLINES || !isr || t->isr[line]) return -1;
    t->isr[line] = isr;
    t->ctx[line] = ctx;
    return 0;
}

void intc_enable(bus_t *b, unsigned line, int on) {
    uint32_t e = b->read(b, ENABLE);
    uint32_t bit = 1u << line;
    b->write(b, ENABLE, on ? (e | bit) : (e & ~bit));
}

int irq_dispatch(bus_t *b, irq_table_t *t) {
    int called = 0;
    for (int pass = 0; pass < MAX_PASSES; pass++) {
        uint32_t pend = b->read(b, PENDING);
        uint32_t todo = pend & b->read(b, ENABLE);
        if (!todo) break;
        for (unsigned line = 0; line < NLINES; line++) {
            if (!(todo & (1u << line))) continue;
            b->write(b, ACK, 1u << line);
            if (t->isr[line]) { t->isr[line](t->ctx[line]); called++; }
            else t->spurious++;
        }
    }
    return called;
}

/* ---- public example ---- */
typedef struct { bus_t bus; uint32_t enable, pending; } demo;
static uint32_t d_read(bus_t *b, uint32_t off) { demo *d = (demo *)b; return off == ENABLE ? d->enable : off == PENDING ? d->pending : 0; }
static void d_write(bus_t *b, uint32_t off, uint32_t v) { demo *d = (demo *)b; if (off == ENABLE) d->enable = v; else if (off == ACK) d->pending &= ~v; }
static uint32_t d_now(bus_t *b) { (void)b; return 0; }
static void on_line(void *ctx) { printf("handler for line %d\n", *(int *)ctx); }

int main(void) {
    demo d = { { d_read, d_write, d_now }, 0, (1u << 1) | (1u << 4) };
    irq_table_t t; memset(&t, 0, sizeof t);
    int one = 1, four = 4;
    irq_register(&t, 1, on_line, &one);
    irq_register(&t, 4, on_line, &four);
    intc_enable(&d.bus, 4, 1);
    printf("ran %d (expect 1: only line 4 is enabled)\n", irq_dispatch(&d.bus, &t));
    printf("line 1 still pending: %s\n", (d.pending & 2) ? "yes" : "no");
    return 0;
}
