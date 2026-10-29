#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

// Provided: sends one character to the serial port (COM1, already configured).
void serial_putc(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0) {
    }
    outb(0x3F8, (uint8_t)c);
}

int ksnprintf(char *buf, size_t size, const char *fmt, ...) {
    return 0;
}

int kprintf(const char *fmt, ...) {
    return 0;
}

// Optional: press Run to see your formatter on the serial port.
void kmain(void) {
    kprintf("Hello %s, you are number %d (0x%x)\r\n", "kernel", 42, 42);
}
