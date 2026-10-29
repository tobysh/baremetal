static char out[128];

#define EXPECT(name, want, ...) do { \
    memset(out, '#', sizeof out); \
    int n_ = ksnprintf(out, sizeof out, __VA_ARGS__); \
    CHECK(name, strcmp_(out, want) == 0 && n_ == (int)strlen_(want), "got \"%s\" (returned %d), expected \"%s\"", out, n_, want); \
} while (0)

static size_t strlen_(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static int strcmp_(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return (unsigned char)*a - (unsigned char)*b; }

// serial loopback so kprintf's output can be read back
static void loopback(void) {
    bm_outb(0x3F8 + 3, 0x03); bm_outb(0x3F8 + 2, 0xC7); bm_outb(0x3F8 + 1, 0);
    bm_outb(0x3F8 + 4, 0x13);
    while (bm_inb(0x3F8 + 5) & 1) bm_inb(0x3F8);
}
static int serial_read(char *b, int max) {
    int n = 0;
    while ((bm_inb(0x3F8 + 5) & 1) && n < max) b[n++] = (char)bm_inb(0x3F8);
    b[n] = 0;
    return n;
}

void kmain(void) {
    EXPECT("plain text is copied unchanged", "hello, kernel", "hello, kernel");
    EXPECT("%c prints a character", "[A]", "[%c]", 'A');
    EXPECT("%s prints a string", "say hi!", "say %s!", "hi");
    EXPECT("%s of NULL prints (null)", "x=(null)", "x=%s", (char *)0);
    EXPECT("%s of an empty string prints nothing", "<>", "<%s>", "");
    EXPECT("%d prints zero", "0", "%d", 0);
    EXPECT("%d prints a positive number", "12345", "%d", 12345);
    EXPECT("%d prints a negative number", "-42", "%d", -42);
    EXPECT("%d handles the most negative int", "-2147483648", "%d", (int)0x80000000);
    EXPECT("%ld handles 64-bit values", "-9000000000", "%ld", -9000000000L);
    EXPECT("%u prints unsigned 32-bit values", "4294967295", "%u", 0xFFFFFFFFu);
    EXPECT("%lu prints the largest 64-bit value", "18446744073709551615", "%lu", 0xFFFFFFFFFFFFFFFFUL);
    EXPECT("%x prints lower-case hex without a prefix", "deadbeef", "%x", 0xDEADBEEFu);
    EXPECT("%x prints 0 as 0", "0", "%x", 0);
    EXPECT("%lx prints 64-bit hex", "123456789abcdef0", "%lx", 0x123456789ABCDEF0UL);
    EXPECT("%p prints 0x and hex", "0xb8000", "%p", (void *)0xB8000);
    EXPECT("%p of a null pointer is 0x0", "0x0", "%p", (void *)0);
    EXPECT("%% prints one percent sign", "100%", "100%%");
    EXPECT("width pads numbers with spaces", "[   42]", "[%5d]", 42);
    EXPECT("width never truncates", "[123456]", "[%3d]", 123456);
    EXPECT("0 flag pads with zeros", "[00042]", "[%05d]", 42);
    EXPECT("zeros go after the minus sign", "[-0042]", "[%05d]", -42);
    EXPECT("spaces go before the minus sign", "[  -42]", "[%5d]", -42);
    EXPECT("%08x gives a fixed-width hex number", "000000ff", "%08x", 0xFF);
    EXPECT("width pads strings with spaces", "[  ab]", "[%4s]", "ab");
    EXPECT("width pads characters", "[  x]", "[%3c]", 'x');
    EXPECT("an unknown conversion is copied through", "a%qb", "a%qb");
    EXPECT("several conversions in a row", "task 7 at 0x1000: ok", "task %d at 0x%x: %s", 7, 0x1000, "ok");
    EXPECT("a number followed by text", "5ms", "%dms", 5);

    // Truncation and the return value.
    char small[8];
    memset(small, '#', sizeof small);
    int n = ksnprintf(small, 6, "%s", "abcdefghij");
    CHECK("a short buffer gets size-1 characters and a NUL", small[0] == 'a' && small[4] == 'e' && small[5] == 0, "buffer is \"%.8s\"", small);
    CHECK("nothing is written beyond the buffer size", small[6] == '#' && small[7] == '#', "bytes after the buffer were changed");
    CHECK("the return value is the full length, not the stored length", n == 10, "returned %d, expected 10", n);

    memset(small, '#', sizeof small);
    n = ksnprintf(small, 0, "%d", 12345);
    CHECK("size 0 writes nothing", small[0] == '#' && n == 5, "first byte is '%c', returned %d", small[0], n);
    n = ksnprintf((char *)0, 0, "len=%d", 100);
    CHECK("a NULL buffer with size 0 only measures", n == 7, "returned %d, expected 7", n);

    memset(small, '#', sizeof small);
    n = ksnprintf(small, 1, "abc");
    CHECK("size 1 stores only the NUL", small[0] == 0 && small[1] == '#' && n == 3, "first byte is %d, second '%c', returned %d", small[0], small[1], n);

    memset(small, '#', sizeof small);
    n = ksnprintf(small, 4, "%05d", 42);
    CHECK("padding is truncated like any other output", small[0] == '0' && small[2] == '0' && small[3] == 0 && n == 5, "buffer is \"%.4s\", returned %d", small, n);

    memset(small, '#', sizeof small);
    ksnprintf(small, 8, "%s", "1234567");
    CHECK("exactly filling the buffer works", small[6] == '7' && small[7] == 0, "buffer is \"%.8s\"", small);

    // kprintf goes to the serial port.
    loopback();
    n = kprintf("v=%d,%s", 5, "ok");
    char got[64];
    int m = serial_read(got, 63);
    CHECK("kprintf sends the formatted text to the serial port", m == 6 && strcmp_(got, "v=5,ok") == 0, "serial received \"%s\" (%d bytes)", got, m);
    CHECK("kprintf returns the length", n == 6, "returned %d", n);

    loopback();
    n = kprintf("%s", "x");
    serial_read(got, 63);
    CHECK("kprintf with a one-character result", got[0] == 'x' && got[1] == 0 && n == 1, "got \"%s\", returned %d", got, n);
}
