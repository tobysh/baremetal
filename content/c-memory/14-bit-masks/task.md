Write four functions that work on one bit of a `uint32_t`. The bit number `n` is between 0 and 31, and bit 0 is the least significant. Every position is tried, on several different starting values.

- `uint32_t set_bit(uint32_t x, int n)` returns `x` with bit `n` turned on.
- `uint32_t clear_bit(uint32_t x, int n)` returns `x` with bit `n` turned off.
- `uint32_t toggle_bit(uint32_t x, int n)` returns `x` with bit `n` flipped.
- `int test_bit(uint32_t x, int n)` returns 1 if bit `n` of `x` is on, otherwise 0 (exactly 1 or 0, not another non-zero number).
