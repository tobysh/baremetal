// These tests use the real UART (in loopback mode, so every byte sent comes straight back)
// and read the real VGA text memory. The receive FIFO holds 16 bytes, so each serial check
// sends fewer than that.
#define LSR_DATA_READY 0x01

static volatile uint16_t *const screen = (volatile uint16_t *)0xB8000;

static void drain(void) { while (bm_inb(COM1 + 5) & LSR_DATA_READY) bm_inb(COM1); }
static void loopback_on(void) { bm_outb(COM1 + 4, 0x13); drain(); }
static int collect(char *buf, int max) {
    int n = 0;
    while ((bm_inb(COM1 + 5) & LSR_DATA_READY) && n < max) buf[n++] = (char)bm_inb(COM1);
    return n;
}

static unsigned cell(int r, int c) { return screen[r * 80 + c]; }

// Does row r of the screen start with exactly the text s (default colour), followed by a blank?
static int row_is(int r, const char *s) {
    int i = 0;
    for (; s[i]; i++) if (cell(r, i) != (0x0700u | (uint8_t)s[i])) return 0;
    return cell(r, i) == 0x0720;
}

static int serial_is(const char *s) {
    char buf[32];
    int n = collect(buf, 32), len = 0;
    while (s[len]) len++;
    if (n != len) return 0;
    for (int i = 0; i < n; i++) if (buf[i] != s[i]) return 0;
    return 1;
}

static void fresh(void) { serial_init(); term_init(); loopback_on(); }

void kmain(void) {
    fresh();
    console_putc('A');
    CHECK("console_putc reaches the screen", cell(0, 0) == 0x0741, "cell (0,0) is 0x%04x", cell(0, 0));
    CHECK("console_putc reaches the serial port", serial_is("A"), "the serial port did not receive exactly \"A\"");

    fresh();
    console_putc('\n');
    CHECK("a newline sends \\r\\n to the serial port", serial_is("\r\n"), "serial output was not exactly \\r\\n");
    console_putc('Z');
    CHECK("a newline moves the screen to the next row, once", cell(1, 0) == 0x075A && cell(0, 0) == 0x0720, "cell (1,0) is 0x%04x", cell(1, 0));

    fresh();
    console_puts("Hi\nyo");
    CHECK("console_puts shows its text across rows", row_is(0, "Hi") && row_is(1, "yo"), "rows are 0x%04x.. and 0x%04x..", cell(0, 0), cell(1, 0));
    drain(); bm_outb(COM1 + 4, 0x13);
    console_puts("Hi\nyo");
    CHECK("console_puts mirrors to the serial port with \\r\\n", serial_is("Hi\r\nyo"), "serial output differs from \"Hi\\r\\nyo\"");

    fresh();
    console_puts("");
    CHECK("console_puts of an empty string does nothing", serial_is("") && row_is(0, ""), "something was printed");

    fresh();
    kprint_dec(0);
    CHECK("kprint_dec(0) shows 0", row_is(0, "0"), "row 0 starts with 0x%04x", cell(0, 0));
    CHECK("kprint_dec(0) goes to the serial port too", serial_is("0"), "serial output was not exactly \"0\"");

    fresh();
    kprint_dec(7);
    CHECK("kprint_dec of a single digit", row_is(0, "7"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    kprint_dec(1234567890);
    CHECK("kprint_dec(1234567890) on the screen", row_is(0, "1234567890"), "row 0 starts with 0x%04x 0x%04x", cell(0, 0), cell(0, 1));
    CHECK("kprint_dec(1234567890) on the serial port", serial_is("1234567890"), "serial output differs");

    fresh();
    kprint_dec(1000);
    CHECK("kprint_dec keeps the zeros inside a number", row_is(0, "1000"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    kprint_dec(4294967296ULL);
    CHECK("kprint_dec handles values above 32 bits", row_is(0, "4294967296"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    kprint_dec(18446744073709551615ULL);
    CHECK("kprint_dec handles the largest 64-bit value (20 digits)", row_is(0, "18446744073709551615"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    kprint_hex(0);
    CHECK("kprint_hex(0) prints 0x0", row_is(0, "0x0"), "row 0 starts with 0x%04x", cell(0, 0));
    CHECK("kprint_hex(0) goes to the serial port too", serial_is("0x0"), "serial output was not exactly \"0x0\"");

    fresh();
    kprint_hex(255);
    CHECK("kprint_hex(255) prints 0xff in lower case", row_is(0, "0xff"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    kprint_hex(0xDEADBEEF);
    CHECK("kprint_hex(0xDEADBEEF) on the screen", row_is(0, "0xdeadbeef"), "row 0 starts with 0x%04x", cell(0, 0));
    CHECK("kprint_hex(0xDEADBEEF) on the serial port", serial_is("0xdeadbeef"), "serial output differs");

    fresh();
    kprint_hex(0x10);
    CHECK("kprint_hex(0x10) keeps its inner zero", row_is(0, "0x10"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    kprint_hex(0xFFFFFFFFFFFFFFFFULL);
    CHECK("kprint_hex handles all 64 bits", row_is(0, "0xffffffffffffffff"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    kprint_hex(0x123456789ABCDEF0ULL);
    CHECK("kprint_hex keeps the order of digits across 64 bits", row_is(0, "0x123456789abcdef0"), "row 0 starts with 0x%04x", cell(0, 0));

    fresh();
    console_puts("n=");
    kprint_dec(42);
    console_putc(' ');
    kprint_hex(42);
    CHECK("numbers print straight after text", row_is(0, "n=42 0x2a"), "row 0 starts with 0x%04x", cell(0, 0));
}
