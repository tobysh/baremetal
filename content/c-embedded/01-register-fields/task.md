Implement `field_get` and `field_set` for a register at byte offset `off` on a `bus_t`.

- `field_get` performs exactly one read and no writes, and the result contains no bits outside the field.
- `field_set` performs exactly one read and exactly one write. Bits outside the field keep the value from that read.
- A `val` wider than the field is truncated to the field's width. It must not leak into neighbouring fields.
- Widths from 1 to 32 are used, including a 32-bit field at shift 0.

Many shifts, widths and old register values are tried.
