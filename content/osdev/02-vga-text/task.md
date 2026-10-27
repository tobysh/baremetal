Video memory is `vga` (80 columns by 25 rows of 16-bit cells) and the current position is `row` and `col`. Implement:

- `uint16_t vga_entry(char c, uint8_t color)` returns a cell: character in the low byte, attribute in the high byte.
- `void term_set_color(uint8_t fg, uint8_t bg)` sets `color` to `fg | (bg << 4)`.
- `void term_init(void)` fills the whole screen with spaces in the current colour, moves to row 0, column 0, and updates the hardware cursor.
- `void term_putc(char c)` handles `\n`, `\r`, `\b`, `\t` and printable characters as the lesson describes, scrolls when needed, and updates the hardware cursor (CRT registers `0x0E` and `0x0F` through ports `0x3D4` and `0x3D5`).
