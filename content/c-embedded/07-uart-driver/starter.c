#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);
    void     (*write)(struct bus *b, uint32_t off, uint32_t v);
    uint32_t (*now)(struct bus *b);
} bus_t;

enum { STATUS = 0x00, DATA = 0x04, CTRL = 0x08 };
enum { RXNE = 1u << 0, TXE = 1u << 1, OVERRUN = 1u << 2, FRAME = 1u << 3, RESETTING = 1u << 4 };
enum { RESET = 1u << 0, RXIE = 1u << 1, TXIE = 1u << 2 };
#define ERR_MASK (OVERRUN | FRAME)
#define UART_RING 8

typedef struct { uint8_t buf[UART_RING]; uint32_t head, tail; } ring_t;
typedef struct { bus_t *bus; ring_t rx, tx; uint32_t overruns, frame_errors, rx_dropped; int txie; } uart_t;

static uint32_t ring_count(const ring_t *r) { (void)r; return 0; }
static int ring_put(ring_t *r, uint8_t c) { (void)r; (void)c; return 0; }
static int ring_get(ring_t *r, uint8_t *c) { (void)r; (void)c; return 0; }

int uart_init(uart_t *u, bus_t *b, uint32_t timeout) {
    (void)timeout;
    memset(u, 0, sizeof *u);
    u->bus = b;
    b->write(b, CTRL, RESET);
    return 0;
}

void uart_isr(uart_t *u) { (void)u; }

size_t uart_write(uart_t *u, const uint8_t *p, size_t n) { (void)u; (void)p; (void)n; return 0; }

int uart_read(uart_t *u, uint8_t *out) { return ring_get(&u->rx, out); }

/* ---- public example: a device that is always ready, holds one byte, loops nothing ---- */
typedef struct { bus_t bus; uint32_t ctrl; int have; uint8_t sent; } demo;
static uint32_t d_read(bus_t *b, uint32_t off) {
    demo *d = (demo *)b;
    if (off == STATUS) return TXE | (d->have ? RXNE : 0);
    if (off == DATA) { d->have = 0; return 'A'; }
    return d->ctrl;
}
static void d_write(bus_t *b, uint32_t off, uint32_t v) {
    demo *d = (demo *)b;
    if (off == CTRL) d->ctrl = v & ~RESET; else if (off == DATA) d->sent = (uint8_t)v;
}
static uint32_t d_now(bus_t *b) { (void)b; return 0; }

int main(void) {
    demo d = { { d_read, d_write, d_now }, 0, 1, 0 };
    uart_t u;
    printf("init %d, CTRL 0x%X (expect 0, 0x2)\n", uart_init(&u, &d.bus, 5), d.ctrl);
    uart_write(&u, (const uint8_t *)"Z", 1);
    uart_isr(&u);                       // receives 'A', sends 'Z'
    uint8_t c = 0;
    printf("read %d '%c', sent '%c' (expect 1 'A', 'Z')\n", uart_read(&u, &c), c, d.sent);
    return 0;
}
