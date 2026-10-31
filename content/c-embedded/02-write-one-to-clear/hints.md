Both functions read STATUS once and keep the value in a local variable instead of reading again.
===
Mask first: `seen = st & (OVERRUN | PARITY | FRAME)`. Only write when `seen != 0`, and write `seen`, not the constant mask.
===
For `uart_getc`, check `st & RXNE`; if set, `return b->read(b, DATA) & 0xFF;`. A second read of DATA discards a byte.
