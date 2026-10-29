Implement:

- `int clamp_byte(long x)` returns `x` limited to the range 0 to 255.
- `size_t count_in_range(const int8_t *p, size_t n, int lo, int hi)` returns how many of the `n` signed bytes satisfy `lo <= p[i] <= hi`. `lo` and `hi` are ordinary signed ints (possibly beyond the byte range).
- `unsigned nibble_at(uint64_t x, unsigned idx)` returns the 4-bit digit at index `idx` (0 to 15).
