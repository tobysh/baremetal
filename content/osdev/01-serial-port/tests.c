// These tests talk to the real (emulated) 16550 on COM1. For the output tests the UART is
// put in loopback mode, where every transmitted byte comes straight back as received data.
#define LSR_DATA_READY 0x01

static uint8_t read_divisor_lo, read_divisor_hi;

static void drain(void) { while (bm_inb(COM1 + 5) & LSR_DATA_READY) bm_inb(COM1); }

// Reads everything sent since the last drain().
static int collect(char *buf, int max) {
    int n = 0;
    while ((bm_inb(COM1 + 5) & LSR_DATA_READY) && n < max) buf[n++] = (char)bm_inb(COM1);
    return n;
}

static void loopback_on(void) { bm_outb(COM1 + 4, 0x13); drain(); }

void kmain(void) {
    // Put the UART in a known-bad state first so we see what serial_init really does.
    bm_outb(COM1 + 3, 0x00); bm_outb(COM1 + 1, 0x0F); bm_outb(COM1 + 4, 0x00);
    serial_init();

    uint8_t lcr = bm_inb(COM1 + 3);
    CHECK("serial_init leaves DLAB clear and 8N1 (line control 0x03)", lcr == 0x03, "line control is 0x%02x", lcr);

    bm_outb(COM1 + 3, 0x80);
    read_divisor_lo = bm_inb(COM1 + 0);
    read_divisor_hi = bm_inb(COM1 + 1);
    bm_outb(COM1 + 3, lcr);
    unsigned div = read_divisor_lo | (read_divisor_hi << 8);
    CHECK("serial_init sets the divisor to 3 (38400 baud)", div == 3, "divisor is %u", div);

    uint8_t ier = bm_inb(COM1 + 1);
    CHECK("serial_init disables UART interrupts", ier == 0, "interrupt enable register is 0x%02x", ier);

    uint8_t iir = bm_inb(COM1 + 2);
    CHECK("serial_init enables the FIFOs", (iir & 0xC0) == 0xC0, "interrupt ID register is 0x%02x (top two bits should be set when FIFOs are on)", iir);

    uint8_t mcr = bm_inb(COM1 + 4);
    CHECK("serial_init raises DTR and RTS", (mcr & 3) == 3, "modem control is 0x%02x", mcr);

    char buf[64];
    loopback_on();
    serial_putc('A');
    int n = collect(buf, 64);
    CHECK("serial_putc sends one byte", n == 1 && buf[0] == 'A', "received %d byte(s)", n);

    loopback_on();
    serial_putc('x'); serial_putc('y'); serial_putc('z');
    n = collect(buf, 64);
    CHECK("serial_putc can be called back to back", n == 3 && buf[0] == 'x' && buf[1] == 'y' && buf[2] == 'z', "received %d byte(s)", n);

    loopback_on();
    serial_puts("Hi!");
    n = collect(buf, 64);
    CHECK("serial_puts sends the whole string and no terminator", n == 3 && buf[0] == 'H' && buf[1] == 'i' && buf[2] == '!', "received %d byte(s)", n);

    loopback_on();
    serial_puts("a\nb");
    n = collect(buf, 64);
    CHECK("serial_puts turns \\n into \\r\\n", n == 4 && buf[0] == 'a' && buf[1] == '\r' && buf[2] == '\n' && buf[3] == 'b', "received %d byte(s)", n);

    loopback_on();
    serial_puts("");
    n = collect(buf, 64);
    CHECK("serial_puts of an empty string sends nothing", n == 0, "received %d byte(s)", n);

    loopback_on();
    serial_puts("\n\n");
    n = collect(buf, 64);
    CHECK("each newline in a row gets its own \\r", n == 4 && buf[0] == '\r' && buf[1] == '\n' && buf[2] == '\r' && buf[3] == '\n', "received %d byte(s)", n);
}
