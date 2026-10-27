#include <stdint.h>
#include <stddef.h>

// Provided: the x86 `out` and `in` instructions.
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

#define VGA_COLS 80
#define VGA_ROWS 25

static volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;
static size_t row, col;
static uint8_t color = 0x07;   // light grey on black

uint16_t vga_entry(char c, uint8_t color) {
    return 0;
}

void term_set_color(uint8_t fg, uint8_t bg) {
}

void term_init(void) {
}

void term_putc(char c) {
}
