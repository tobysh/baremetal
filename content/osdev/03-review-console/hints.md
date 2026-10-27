`console_putc` is three calls and one `if`: when `c == '\n'`, call `serial_putc('\r')`; then always `serial_putc(c)` and `term_putc(c)`. `console_puts` is a loop over the string that calls `console_putc` for each character (stop at the `\0`, do not send it).
===
For `kprint_dec`, fill a `char buf[20]` from the end with a `do { buf[--i] = '0' + n % 10; n /= 10; } while (n != 0);` loop, then send `buf[i]` through `buf[19]` to `console_putc`. Use `do`/`while` rather than `while` so that 0 still prints a digit.
===
`kprint_hex` is the same idea with `n & 0xF` and `n >>= 4`. A digit below 10 is `'0' + d`; otherwise it is `'a' + (d - 10)`. Print `'0'` and `'x'` first and then the buffer, which needs 16 characters for a full 64-bit value.
===
```c
void kprint_hex(uint64_t n) {
    char buf[16];
    int i = 16;
    do {
        uint8_t d = n & 0xF;
        buf[--i] = d < 10 ? '0' + d : 'a' + (d - 10);
        n >>= 4;
    } while (n != 0);
    console_putc('0');
    console_putc('x');
    for (; i < 16; i++) console_putc(buf[i]);
}
```
