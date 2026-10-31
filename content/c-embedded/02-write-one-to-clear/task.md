Implement two functions on a `bus_t` UART (STATUS at 0x00, DATA at 0x04):

- `uint32_t uart_clear_errors(bus_t *b)` reads STATUS once, and if any of OVERRUN, PARITY or FRAME are set, writes STATUS with **exactly those observed error bits** and returns them. If none are set it returns 0 and performs no write. A flag raised after your read must still be pending afterwards.
- `int uart_getc(bus_t *b)` reads STATUS once. If RXNE is clear it returns -1 and does not touch DATA. Otherwise it reads DATA **exactly once** and returns the byte (0 to 255).

Different error combinations, late-arriving flags and FIFO contents are tried.
