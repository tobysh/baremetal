Implement:

- `uint64_t get_bits(uint64_t x, unsigned lo, unsigned len)` returns the `len` bits of `x` starting at bit `lo` (bit 0 is the least significant), shifted down to bit 0. `lo` and `len` are below 64, and `len` may be 0.
- `uint32_t rotl32(uint32_t x, unsigned n)` rotates the 32-bit value left by `n % 32` bits.
- `uint64_t set_bits(uint64_t x, unsigned lo, unsigned len, uint64_t v)` returns `x` with the field replaced by the low `len` bits of `v`. `lo + len` is at most 64. Bits of `v` above `len` must be ignored.
