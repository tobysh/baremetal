typedef struct {
    bus_t bus; uint32_t clock;
    uint32_t ctrl, err, reset_until, reset_ticks, tx_busy_until, stale;
    uint8_t fifo[4]; int fn;
    uint8_t txlog[64]; int ntx, tx_lost, ctrl_writes, accesses, err_writes; uint32_t last_err_write; int resetting_flag;
} D;
static void dv_upd(D *d) {
    if (d->resetting_flag && d->clock >= d->reset_until) { d->resetting_flag = 0; d->ctrl = 0; d->fn = 0; d->err = d->stale; }
}
static uint32_t dv_read(bus_t *b, uint32_t off) {
    D *d = (D *)b; d->accesses++; dv_upd(d);
    if (off == 0x00) {
        uint32_t v = (d->fn ? 1u : 0u) | (d->clock >= d->tx_busy_until ? 2u : 0u) | d->err | (d->resetting_flag ? 16u : 0u);
        d->clock++;
        return v;
    }
    if (off == 0x04) { uint32_t v = 0x5A5A5A00u; if (d->fn) { v |= d->fifo[0]; memmove(d->fifo, d->fifo + 1, 3); d->fn--; } return v; }
    return d->ctrl;
}
static void dv_write(bus_t *b, uint32_t off, uint32_t v) {
    D *d = (D *)b; d->accesses++; dv_upd(d);
    if (off == 0x00) { d->err_writes++; d->last_err_write = v; d->err &= ~(v & 12u); }
    else if (off == 0x04) {
        if (d->clock >= d->tx_busy_until && !d->resetting_flag) { if (d->ntx < 64) d->txlog[d->ntx] = (uint8_t)v; d->ntx++; d->tx_busy_until = d->clock + 2; }
        else d->tx_lost++;
    } else {
        d->ctrl_writes++;
        if (d->resetting_flag) return;
        if (v & 1u) { d->resetting_flag = 1; d->reset_until = d->clock + d->reset_ticks; }
        else d->ctrl = v & 6u;
    }
}
static uint32_t dv_now(bus_t *b) { return ((D *)b)->clock; }
static void dv_init(D *d, uint32_t reset_ticks, uint32_t stale) {
    memset(d, 0, sizeof *d);
    d->bus.read = dv_read; d->bus.write = dv_write; d->bus.now = dv_now;
    d->reset_ticks = reset_ticks; d->stale = stale;
}
static int dv_line(D *d) {
    dv_upd(d);
    if (d->resetting_flag) return 0;
    return ((d->ctrl & 2u) && d->fn) || ((d->ctrl & 4u) && d->clock >= d->tx_busy_until) || d->err != 0;
}
static int dv_feed(D *d, uint8_t c) {   // returns 0 if the FIFO overflowed
    if (d->fn >= 4) { d->err |= 4u; return 0; }
    d->fifo[d->fn++] = c; return 1;
}
static void dv_pump(D *d, uart_t *u, int ticks) {
    for (int i = 0; i < ticks; i++) { d->clock++; for (int k = 0; k < 6 && dv_line(d); k++) uart_isr(u); }
}
static int up(D *d, uart_t *u) {   // standard init
    dv_init(d, 3, 0); return uart_init(u, &d->bus, 10);
}
static void drain_to(uart_t *u, uint8_t *buf, int *n) { *n = 0; uint8_t c; while (uart_read(u, &c) && *n < 64) buf[(*n)++] = c; }

int main(void) {
    D d; uart_t u; uint8_t buf[64]; int n, r;

    // ---- init
    int ok = 1; char why[120] = "";
    static const uint32_t rt[] = { 0, 3, 8, 10 };
    for (int i = 0; i < 4; i++) {
        dv_init(&d, rt[i], 4u | 8u);
        r = uart_init(&u, &d.bus, 10);
        if (r != 0 || d.ctrl != 2u || d.err != 0 || u.overruns || u.frame_errors || u.rx_dropped || dv_line(&d) || u.txie) { ok = 0; snprintf(why, sizeof why, "reset %u ticks: returned %d, CTRL 0x%X, flags 0x%X, counters %u/%u", rt[i], r, d.ctrl, d.err, u.overruns, u.frame_errors); }
    }
    CHECK("init resets, clears stale flags without counting them, enables RXIE", ok, "%s", why);
    dv_init(&d, 100, 0);
    r = uart_init(&u, &d.bus, 10);
    CHECK("init gives up with -1 on a reset that takes too long", r == -1, "returned %d", r);
    CHECK("a failed init reads STATUS a bounded number of times", d.accesses <= 16, "%d register accesses", d.accesses);
    dv_init(&d, 11, 0);
    CHECK("a reset slower than the timeout is also refused", uart_init(&u, &d.bus, 5) == -1, "slow reset was accepted");

    // ---- receive
    up(&d, &u);
    const char *msg = "hello";
    for (int i = 0; i < 2; i++) dv_feed(&d, (uint8_t)msg[i]);
    dv_pump(&d, &u, 2);
    for (int i = 2; i < 5; i++) dv_feed(&d, (uint8_t)msg[i]);
    dv_pump(&d, &u, 2);
    drain_to(&u, buf, &n);
    CHECK("bytes arrive in order", n == 5 && memcmp(buf, "hello", 5) == 0, "got %d bytes", n);
    CHECK("the interrupt line is low after the ISR ran", !dv_line(&d), "line still high (flags 0x%X)", d.err);
    CHECK("uart_read on an empty driver returns 0 without touching the bus", (d.accesses = 0, uart_read(&u, buf) == 0 && d.accesses == 0), "returned data or used the bus");

    up(&d, &u);
    u.rx.head = u.rx.tail = 0xFFFFFFFDu;
    ok = 1;
    int total = 0;
    for (int burst = 0; burst < 4; burst++) {
        for (int i = 0; i < 3; i++) dv_feed(&d, (uint8_t)(0x40 + burst * 3 + i));
        dv_pump(&d, &u, 2);
        if (burst == 1) continue;       // let two bursts (6 bytes) pile up on burst 1 and 2
        drain_to(&u, buf + total, &n); total += n;
    }
    n = total;
    for (int i = 0; i < 12; i++) if (n != 12 || buf[i] != 0x40 + i) ok = 0;
    CHECK("the receive ring wraps correctly across the 32-bit counter limit", ok, "got %d bytes, first 0x%02X", n, buf[0]);

    up(&d, &u);
    for (int burst = 0; burst < 4; burst++) { for (int i = 0; i < 4; i++) dv_feed(&d, (uint8_t)(0x10 * (burst + 1) + i)); dv_pump(&d, &u, 2); }
    drain_to(&u, buf, &n);
    ok = n == 8;
    for (int i = 0; i < 8 && ok; i++) if (buf[i] != 0x10 * (i / 4 + 1) + i % 4) ok = 0;
    CHECK("a full ring keeps the oldest 8 bytes and drops the newest", ok, "kept %d bytes", n);
    CHECK("dropped bytes are counted", u.rx_dropped == 8 && u.overruns == 0, "rx_dropped %u (expected 8), overruns %u", u.rx_dropped, u.overruns);

    up(&d, &u);
    for (int i = 0; i < 7; i++) dv_feed(&d, (uint8_t)(0x30 + i));      // 3 bytes overflow the 4-deep FIFO
    dv_pump(&d, &u, 2);
    drain_to(&u, buf, &n);
    CHECK("a hardware FIFO overrun is counted once and the survivors are kept", u.overruns == 1 && n == 4 && buf[0] == 0x30 && buf[3] == 0x33, "overruns %u, %d bytes", u.overruns, n);
    CHECK("the overrun flag was acknowledged with exactly that bit", d.err == 0 && d.last_err_write == 4u && d.err_writes >= 1, "flags 0x%X, last STATUS write 0x%X", d.err, d.last_err_write);
    for (int i = 0; i < 6; i++) dv_feed(&d, 0x77);
    dv_pump(&d, &u, 2);
    CHECK("a second overrun is counted again", u.overruns == 2, "overruns %u", u.overruns);

    up(&d, &u);
    d.err |= 8u; dv_pump(&d, &u, 1);
    d.err |= 8u; d.err |= 4u; dv_pump(&d, &u, 1);
    CHECK("frame errors are counted per event and cleared", u.frame_errors == 2 && u.overruns == 1 && d.err == 0 && !dv_line(&d), "frame %u, overruns %u, flags 0x%X", u.frame_errors, u.overruns, d.err);

    // ---- transmit
    up(&d, &u);
    r = (int)uart_write(&u, (const uint8_t *)"HELLO", 5);
    CHECK("uart_write returns the number queued", r == 5, "returned %d", r);
    CHECK("queueing a byte sets TXIE and keeps RXIE", d.ctrl == (2u | 4u) && u.txie, "CTRL 0x%X", d.ctrl);
    dv_pump(&d, &u, 40);
    CHECK("bytes leave in order and none are lost", d.ntx == 5 && memcmp(d.txlog, "HELLO", 5) == 0 && d.tx_lost == 0, "%d sent, %d lost", d.ntx, d.tx_lost);
    CHECK("TXIE is cleared when the ring drains and RXIE survives", d.ctrl == 2u && !u.txie && !dv_line(&d), "CTRL 0x%X, txie %d", d.ctrl, u.txie);

    up(&d, &u);
    uint8_t big[20]; for (int i = 0; i < 20; i++) big[i] = (uint8_t)('a' + i);
    r = (int)uart_write(&u, big, 20);
    CHECK("a write larger than the ring queues exactly 8", r == 8, "returned %d", r);
    dv_pump(&d, &u, 60);
    CHECK("the 8 queued bytes are the first 8, in order", d.ntx == 8 && memcmp(d.txlog, big, 8) == 0 && d.tx_lost == 0, "%d sent, %d lost", d.ntx, d.tx_lost);
    r = (int)uart_write(&u, big + 8, 3);
    dv_pump(&d, &u, 30);
    CHECK("the driver keeps working after draining", r == 3 && d.ntx == 11 && memcmp(d.txlog + 8, big + 8, 3) == 0, "returned %d, %d sent", r, d.ntx);

    up(&d, &u);
    int a1 = (int)uart_write(&u, big, 6), a2 = (int)uart_write(&u, big + 6, 6);
    CHECK("a second write only gets the space that is left", a1 == 6 && a2 == 2, "returned %d then %d", a1, a2);
    dv_pump(&d, &u, 60);
    CHECK("partial writes keep the right bytes", d.ntx == 8 && memcmp(d.txlog, big, 8) == 0, "%d sent", d.ntx);

    up(&d, &u);
    u.tx.head = u.tx.tail = 0xFFFFFFFEu;
    r = (int)uart_write(&u, big, 6);
    dv_pump(&d, &u, 40);
    CHECK("the transmit ring wraps across the 32-bit counter limit", r == 6 && d.ntx == 6 && memcmp(d.txlog, big, 6) == 0, "returned %d, %d sent", r, d.ntx);

    up(&d, &u);
    int w0 = d.ctrl_writes;
    r = (int)uart_write(&u, big, 0);
    CHECK("writing nothing touches no register", r == 0 && d.ctrl_writes == w0 && !u.txie, "returned %d", r);

    // ---- full duplex
    up(&d, &u);
    uart_write(&u, (const uint8_t *)"ping", 4);
    for (int i = 0; i < 3; i++) { dv_feed(&d, (uint8_t)('x' + i)); dv_pump(&d, &u, 4); }
    dv_pump(&d, &u, 30);
    drain_to(&u, buf, &n);
    CHECK("receive and transmit work together", n == 3 && memcmp(buf, "xyz", 3) == 0 && d.ntx == 4 && memcmp(d.txlog, "ping", 4) == 0 && d.tx_lost == 0,
          "%d received, %d sent, %d lost", n, d.ntx, d.tx_lost);
    CHECK("the interrupt line ends low and CTRL back to RXIE", !dv_line(&d) && d.ctrl == 2u, "CTRL 0x%X", d.ctrl);
    return 0;
}
