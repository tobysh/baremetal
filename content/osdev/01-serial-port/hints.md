Each register is `COM1 + n`. The divisor only lands in the right place while bit 7 of `COM1 + 3` is set, so write `0x80` there first, then the two divisor bytes, then `0x03` to leave DLAB.
===
`serial_putc` is a loop that reads `inb(COM1 + 5)` and keeps going while `(status & 0x20) == 0`. Only after the loop do you `outb(COM1, c)`.
===
```c
void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 3);
    outb(COM1 + 1, 0);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x03);
}
```
For `serial_puts`, loop with `for (; *s; s++)` and call `serial_putc('\r')` before `serial_putc('\n')`.
