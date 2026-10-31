Implement `irq_register`, `intc_enable` and `irq_dispatch` for the controller in the lesson.

- `irq_register`: 0 on success; -1 for line >= 8, a NULL handler, or an already registered line.
- `intc_enable(b, line, on)`: one read and one write of ENABLE; other bits are preserved.
- `irq_dispatch`: up to 16 passes. Each pass: read PENDING and ENABLE once; if `pending & enabled` is 0 stop; else for each set line in ascending order write ACK with only that line's bit, then call its handler (or count `spurious`). Return how many handlers ran.
- Lines that are pending but masked must stay pending and un-acknowledged.

Handlers that raise new events while they run, stuck lines, masked lines and lines with no handler are tried.
