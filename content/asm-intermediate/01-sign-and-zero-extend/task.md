Implement three functions that sum an array in different element types and return a 64-bit result:

- `long sum_i8(const int8_t *p, size_t n)` sums `n` signed bytes.
- `unsigned long sum_u8(const uint8_t *p, size_t n)` sums `n` unsigned bytes.
- `long sum_i32(const int32_t *p, size_t n)` sums `n` signed 32-bit integers into a 64-bit total (no 32-bit wraparound).

`n` may be 0. Several different arrays are tried.
