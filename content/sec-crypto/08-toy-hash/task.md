Implement `uint8_t tiny_hash(const uint8_t *p,size_t n)` using the FNV-1a constants above and low-eight-bit output. Implement `int first_collision(uint16_t *a,uint16_t *b)`: enumerate v=0..65535 as two little-endian bytes and return the first repeated bucket with its earliest value in *a and current value in *b. Return 1 on success, 0 for a NULL output.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
