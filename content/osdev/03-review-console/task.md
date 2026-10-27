Your `serial_*` and `term_*` drivers are provided in the starter. Implement:

- `void console_putc(char c)` sends `c` to both the serial port and the screen. For `'\n'`, send `'\r'` to the serial port first (the screen is sent just the `'\n'`).
- `void console_puts(const char *s)` sends every character of `s` through `console_putc`.
- `void kprint_dec(uint64_t n)` prints `n` in decimal through `console_putc`, with no leading zeros. `0` prints as `0`.
- `void kprint_hex(uint64_t n)` prints `n` as `0x` followed by lowercase hex digits with no leading zeros. `0` prints as `0x0`.

Neither print function may print a newline or anything else beyond the digits.
