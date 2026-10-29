Implement `int copy_bytes(uint8_t *dst, size_t cap, const uint8_t *src, size_t n)`. Return 1 on success, 0 on rejection. Reject `n > cap` or a NULL pointer for nonzero `n`. Copy exactly `n` bytes, without a terminator. Failure writes nothing; `n == 0` succeeds.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
