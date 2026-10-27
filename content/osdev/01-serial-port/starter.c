#include <stdint.h>
#include <stddef.h>

// Provided: the x86 `out` and `in` instructions.
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

#define COM1 0x3F8

void serial_init(void) {
}

void serial_putc(char c) {
}

void serial_puts(const char *s) {
}

// Optional: press Run to see your text.
void kmain(void) {
    serial_init();
    serial_puts("Hello from my kernel!\n");
}
