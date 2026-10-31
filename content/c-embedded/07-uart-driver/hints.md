Write the ring first: `ring_count = head - tail` (unsigned), put refuses at `UART_RING`, and both indices are masked only when addressing `buf`.
===
`uart_init`: write RESET, then loop reading STATUS until RESETTING is clear, giving up when more than `timeout` ticks have elapsed since entry. Afterwards read STATUS once more and write back only `st & (OVERRUN | FRAME)`.
===
`uart_isr`: keep one local `st`. Handle errors from the first read, then `for (n = 0; n < 16 && (st & RXNE); n++)` read DATA, push or count a drop, and refresh `st`. Use the final `st` for the TXE decision.
===
Only write DATA when TXE was set in `st`. Writing while TXE is low loses the byte, and the simulator counts it.
