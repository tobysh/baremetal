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

static uint32_t ring_count(const ring_t *r) { return r->head - r->tail; }
static int ring_put(ring_t *r, uint8_t c) {
    if (ring_count(r) >= UART_RING) return 0;
    r->buf[r->head++ & (UART_RING - 1)] = c;
    return 1;
}
static int ring_get(ring_t *r, uint8_t *c) {
    if (ring_count(r) == 0) return 0;
    *c = r->buf[r->tail++ & (UART_RING - 1)];
    return 1;
}

static void set_ctrl_bit(uart_t *u, uint32_t bit, int on) {
    uint32_t c = u->bus->read(u->bus, CTRL);
    u->bus->write(u->bus, CTRL, on ? (c | bit) : (c & ~bit));
}

int uart_init(uart_t *u, bus_t *b, uint32_t timeout) {
    memset(u, 0, sizeof *u);
    u->bus = b;
    uint32_t start = b->now(b);
    b->write(b, CTRL, RESET);
    for (;;) {
        if (!(b->read(b, STATUS) & RESETTING)) break;
        if ((uint32_t)(b->now(b) - start) > timeout) return -1;
    }
    uint32_t stale = b->read(b, STATUS) & ERR_MASK;
    if (stale) b->write(b, STATUS, stale);
    set_ctrl_bit(u, RXIE, 1);
    return 0;
}

void uart_isr(uart_t *u) {
    bus_t *b = u->bus;
    uint32_t st = b->read(b, STATUS);
    uint32_t err = st & ERR_MASK;
    if (err) {
        if (err & OVERRUN) u->overruns++;
        if (err & FRAME) u->frame_errors++;
        b->write(b, STATUS, err);
    }
    for (int n = 0; n < 16 && (st & RXNE); n++) {
        uint8_t c = (uint8_t)(b->read(b, DATA) & 0xFF);
        if (!ring_put(&u->rx, c)) u->rx_dropped++;
        st = b->read(b, STATUS);
    }
    if (st & TXE) {
        uint8_t c;
        if (ring_get(&u->tx, &c)) b->write(b, DATA, c);
        else if (u->txie) { set_ctrl_bit(u, TXIE, 0); u->txie = 0; }
    }
}

size_t uart_write(uart_t *u, const uint8_t *p, size_t n) {
    size_t done = 0;
    while (done < n && ring_put(&u->tx, p[done])) done++;
    if (done && !u->txie) { set_ctrl_bit(u, TXIE, 1); u->txie = 1; }
    return done;
}

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
