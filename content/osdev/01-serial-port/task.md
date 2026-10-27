`outb`, `inb` and `COM1` are provided. Implement:

- `void serial_init(void)` programs the UART for **38400 baud (divisor 3), 8N1, interrupts disabled, FIFOs enabled and cleared, DTR and RTS raised**, in an order the hardware accepts.
- `void serial_putc(char c)` waits until the transmitter is empty (line status bit 5), then sends `c`. Never write while the UART is busy.
- `void serial_puts(const char *s)` sends every character of `s` (not the terminating `\0`), sending `\r` before each `\n`.
