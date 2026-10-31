#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);
    void     (*write)(struct bus *b, uint32_t off, uint32_t v);
    uint32_t (*now)(struct bus *b);
} bus_t;

enum { STATUS = 0x00, DATA = 0x04 };
enum { RXNE = 1u << 0, TXE = 1u << 1, OVERRUN = 1u << 2, PARITY = 1u << 3, FRAME = 1u << 4 };
#define ERR_MASK (OVERRUN | PARITY | FRAME)

// Acknowledge error flags. Returns the error bits that were set.
uint32_t uart_clear_errors(bus_t *b) {
    uint32_t st = b->read(b, STATUS);
    uint32_t seen = st & ERR_MASK;
    if (seen) b->write(b, STATUS, seen);
    return seen;
}

// Returns a received byte, or -1 if none is waiting.
int uart_getc(bus_t *b) {
    if (!(b->read(b, STATUS) & RXNE)) return -1;
    return b->read(b, DATA) & 0xFF;
}

/* ---- public example: a tiny scripted UART ---- */
typedef struct { bus_t bus; uint32_t err; uint8_t fifo[4]; int n; } demo;
static uint32_t d_read(bus_t *b, uint32_t off) {
    demo *d = (demo *)b;
    if (off == STATUS) return (d->n ? RXNE : 0) | TXE | d->err;
    uint32_t v = 0; if (d->n) { v = d->fifo[0]; memmove(d->fifo, d->fifo + 1, 3); d->n--; }
    return v;
}
static void d_write(bus_t *b, uint32_t off, uint32_t v) { demo *d = (demo *)b; if (off == STATUS) d->err &= ~(v & ERR_MASK); }
static uint32_t d_now(bus_t *b) { (void)b; return 0; }

int main(void) {
    demo d = { { d_read, d_write, d_now }, OVERRUN | FRAME, { 'h', 'i' }, 2 };
    printf("cleared 0x%X (expect 0x14)\n", uart_clear_errors(&d.bus));
    printf("left    0x%X (expect 0x0)\n", d.err);
    printf("%c%c then %d (expect hi then -1)\n", uart_getc(&d.bus), uart_getc(&d.bus), uart_getc(&d.bus));
    return 0;
}
