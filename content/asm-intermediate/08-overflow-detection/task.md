Each function returns 1 on success and 0 on overflow. On overflow `*out` is left untouched.

- `int add_checked(long a, long b, long *out)` stores `a + b` unless the signed sum overflows.
- `int mul_checked_u(uint64_t a, uint64_t b, uint64_t *out)` stores `a * b` unless the unsigned product does not fit in 64 bits.
- `int fact_checked(unsigned n, uint64_t *out)` stores `n!` (with `0! = 1`) unless it does not fit in 64 bits. Multiply in a loop and check every step.
