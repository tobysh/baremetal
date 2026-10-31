Implement `ring_*` helpers, `uart_init`, `uart_isr`, `uart_write` and `uart_read` exactly as the lesson describes. Do not change the `ring_t` and `uart_t` definitions; the tests read and set their fields (including counters near 2^32).

Checked behaviours include:
- init: bounded reset wait (-1 on timeout), stale errors cleared and not counted, RXIE set, other CTRL bits kept.
- receive: bytes arrive in order, the ring wraps, a full ring drops the newest byte and counts it, FIFO overruns and frame errors are counted once per event and acknowledged with exactly the observed bits, the interrupt line is low after the ISR.
- transmit: `uart_write` returns how many fit, bytes go out in order with none lost on the wire, TXIE is set when needed and cleared when the ring drains, RXIE survives, writing nothing touches no register.
