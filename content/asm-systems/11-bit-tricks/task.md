Implement three functions on unsigned 64-bit values:

- `int bit_count(unsigned long x)` returns the number of set bits (use `popcnt`).
- `int lowest_set_index(unsigned long x)` returns the index of the lowest set bit, or `-1` if `x` is 0 (use `bsf`).
- `unsigned long next_pow2(unsigned long n)` returns the smallest power of two that is `>= n`; for `n = 0` and `n = 1` return 1. Inputs never exceed 2^63. Use `bsr`.

The argument is in `rdi`; results in `eax` / `rax`. Preserve `rbx`, `rbp` and `r12`-`r15`.
