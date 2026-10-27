`vga_entry` is `(uint16_t)((uint16_t)color << 8 | (uint8_t)c)`. Cast the character to `uint8_t` first so a negative `char` doesn't smear its sign bits into the attribute byte.
===
Write a helper `newline()` that does `col = 0; row++;` and then scrolls if `row == VGA_ROWS`. Use it from `\n`, from tab overflow and from wrapping at column 80.
===
To scroll, copy `vga[(r + 1) * VGA_COLS + c]` into `vga[r * VGA_COLS + c]` for rows 0 to 23, fill row 24 with `vga_entry(' ', color)`, and set `row = VGA_ROWS - 1`.
===
To set the cursor, compute `pos = row * VGA_COLS + col`, then `outb(0x3D4, 0x0F); outb(0x3D5, pos & 0xFF); outb(0x3D4, 0x0E); outb(0x3D5, pos >> 8);`. Call it at the end of `term_init` and `term_putc`.
