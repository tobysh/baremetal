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

// Optional: press Run to see your text.
void kmain(void) {
    serial_init();
    serial_puts("Hello from my kernel!\n");
}
