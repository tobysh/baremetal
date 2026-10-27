#include <stdint.h>
#include <stddef.h>

// Provided: the x86 `out` and `in` instructions.
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00);   // no interrupts
    outb(COM1 + 3, 0x80);   // DLAB on: registers 0/1 become the divisor
    outb(COM1 + 0, 3);      // divisor low byte: 115200 / 3 = 38400 baud
    outb(COM1 + 1, 0);      // divisor high byte
    outb(COM1 + 3, 0x03);   // DLAB off, 8 data bits, no parity, 1 stop bit
    outb(COM1 + 2, 0xC7);   // enable and clear FIFOs
    outb(COM1 + 4, 0x03);   // DTR + RTS
}

void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0) {
    }
    outb(COM1, (uint8_t)c);
}

void serial_puts(const char *s) {
    for (; *s; s++) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s);
    }
}

#define VGA_COLS 80
#define VGA_ROWS 25

static volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;
static size_t row, col;
static uint8_t color = 0x07;   // light grey on black

uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)(((uint16_t)color << 8) | (uint8_t)c);
}

void term_set_color(uint8_t fg, uint8_t bg) {
    color = (uint8_t)(fg | (bg << 4));
}

static void update_cursor(void) {
    uint16_t pos = (uint16_t)(row * VGA_COLS + col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(pos >> 8));
}

void term_init(void) {
    for (size_t i = 0; i < VGA_COLS * VGA_ROWS; i++) vga[i] = vga_entry(' ', color);
    row = col = 0;
    update_cursor();
}

static void newline(void) {
    col = 0;
    if (++row < VGA_ROWS) return;
    for (size_t r = 0; r < VGA_ROWS - 1; r++)
        for (size_t c = 0; c < VGA_COLS; c++) vga[r * VGA_COLS + c] = vga[(r + 1) * VGA_COLS + c];
    for (size_t c = 0; c < VGA_COLS; c++) vga[(VGA_ROWS - 1) * VGA_COLS + c] = vga_entry(' ', color);
    row = VGA_ROWS - 1;
}

void term_putc(char c) {
    if (c == '\n') {
        newline();
    } else if (c == '\r') {
        col = 0;
    } else if (c == '\b') {
        if (col > 0) {
            col--;
            vga[row * VGA_COLS + col] = vga_entry(' ', color);
        }
    } else if (c == '\t') {
        col = (col + 8) & ~(size_t)7;
        if (col >= VGA_COLS) newline();
    } else {
        vga[row * VGA_COLS + col] = vga_entry(c, color);
        if (++col == VGA_COLS) newline();
    }
    update_cursor();
}

// ---- New in this lesson ----

void console_putc(char c) {
}

void console_puts(const char *s) {
}

void kprint_dec(uint64_t n) {
}

void kprint_hex(uint64_t n) {
}

// Optional: press Run to see your console. Run shows the serial port; the same
// text is also written to video memory.
void kmain(void) {
    serial_init();
    term_init();
    console_puts("Hello, console!\n");
    console_puts("dec: ");
    kprint_dec(1234567890);
    console_putc('\n');
    console_puts("hex: ");
    kprint_hex(0xDEADBEEF);
    console_putc('\n');
}
