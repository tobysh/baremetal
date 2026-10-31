Implement `int wait_flag(bus_t *b, uint32_t off, uint32_t mask, uint32_t timeout)`.

- Read the register at `off`. If `(value & mask) == mask`, return 0.
- Otherwise read `b->now(b)`; if more than `timeout` ticks have passed since entry (unsigned subtraction, so a wrapping counter works), return -1. Else read again.
- The register is always read at least once, even if `timeout` is 0. Never write to the device.
- A device that is never ready must cost at most `timeout + 1` reads.

Different start times (including near 0xFFFFFFFF), bus speeds, ready times and timeouts are tried.
