Implement `int sensor_read_mC(bus_t *b, int32_t *out, uint32_t timeout)` for the sensor in the lesson.

- Write START once; poll STATUS with the bounded-wait rule (read first, give up when more than `timeout` ticks have elapsed since entry).
- FAULT: write exactly the FAULT bit to STATUS and return -2. Timeout: return -1. On both, leave `*out` alone and do not read DATA.
- DONE: read DATA once, sign-extend its low 12 bits, convert 1/16 C units to millidegrees (nearest, ties away from zero), store, return 0.
- The driver must work again on the next call after a fault.

Many readings (including the extremes), latencies and faults are tried, with junk in the upper DATA bits.
