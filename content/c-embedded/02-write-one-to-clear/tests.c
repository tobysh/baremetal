typedef struct { bus_t bus; uint32_t err, inject; int q[16], rh, rn, sreads, swrites, dreads; uint32_t lastw; } U;
static uint32_t u_read(bus_t *b, uint32_t off) {
    U *u = (U *)b;
    if (off == STATUS) {
        uint32_t v = (u->rh < u->rn ? RXNE : 0) | TXE | u->err;
        u->sreads++;
        u->err |= u->inject; u->inject = 0;
        return v;
    }
    u->dreads++;
    if (u->rh < u->rn) return 0x5A5A5A00u | (uint32_t)u->q[u->rh++];
    return 0;
}
static void u_write(bus_t *b, uint32_t off, uint32_t v) {
    U *u = (U *)b;
    if (off == STATUS) { u->swrites++; u->lastw = v; u->err &= ~(v & ERR_MASK); }
}
static uint32_t u_now(bus_t *b) { (void)b; return 0; }
static void u_init(U *u, uint32_t err) {
    memset(u, 0, sizeof *u);
    u->bus.read = u_read; u->bus.write = u_write; u->bus.now = u_now; u->err = err;
}

int main(void) {
    static const uint32_t combos[] = { OVERRUN, PARITY, FRAME, OVERRUN | PARITY, PARITY | FRAME, ERR_MASK };
    int ret_ok = 1, w_ok = 1, cleared_ok = 1, reads_ok = 1; uint32_t badr = 0;
    for (unsigned i = 0; i < sizeof combos / sizeof combos[0]; i++) {
        U u; u_init(&u, combos[i]);
        uint32_t r = uart_clear_errors(&u.bus);
        if (r != combos[i]) { ret_ok = 0; badr = r; }
        if (u.swrites != 1 || (u.lastw & ERR_MASK) != combos[i]) w_ok = 0;
        if (u.err != 0) cleared_ok = 0;
        if (u.sreads != 1) reads_ok = 0;
    }
    CHECK("returns the error bits it saw", ret_ok, "returned 0x%X for one of the combinations", badr);
    CHECK("writes exactly the observed bits, once", w_ok, "wrong number of writes or wrong bits written");
    CHECK("all observed flags end up cleared", cleared_ok, "a flag was still set");
    CHECK("STATUS is read once", reads_ok, "STATUS read more than once");

    U u; u_init(&u, 0);
    uint32_t r = uart_clear_errors(&u.bus);
    CHECK("no errors: returns 0 and does not write", r == 0 && u.swrites == 0, "returned 0x%X after %d writes", r, u.swrites);

    u_init(&u, PARITY); u.inject = FRAME;
    r = uart_clear_errors(&u.bus);
    CHECK("a flag raised after the read survives", r == PARITY && (u.err & FRAME) && !(u.err & PARITY),
          "returned 0x%X, flags now 0x%X (a late FRAME must stay pending)", r, u.err);

    u_init(&u, 0); u.inject = OVERRUN;
    uart_clear_errors(&u.bus);
    CHECK("an idle device is not acknowledged blindly", (u.err & OVERRUN) != 0 && u.swrites == 0, "a flag that was never seen was cleared");

    u_init(&u, 0);
    CHECK("empty FIFO gives -1 without reading DATA", uart_getc(&u.bus) == -1 && u.dreads == 0, "dreads=%d", u.dreads);

    u_init(&u, 0);
    int bytes[] = { 0x00, 0x41, 0x7F, 0xFF, 0x80, 0x0A, 0x31, 0xE9 };
    u.rn = 8; for (int i = 0; i < 8; i++) u.q[i] = bytes[i];
    int ok = 1, got = 0;
    for (int i = 0; i < 8; i++) { got = uart_getc(&u.bus); if (got != bytes[i]) { ok = 0; break; } }
    CHECK("bytes come out in order, 0 to 255, upper bits masked", ok, "byte %d: got %d", 0, got);
    CHECK("DATA was read once per byte", u.dreads == 8, "%d DATA reads for 8 bytes", u.dreads);
    CHECK("then the FIFO is empty again", uart_getc(&u.bus) == -1 && u.dreads == 8, "dreads=%d", u.dreads);
    return 0;
}
