Implement two functions:

- `void prefix_sums(long *a, size_t n)` replaces `a[i]` with `a[0] + ... + a[i]` for every `i`, in place.
- `long trace(const int *m, size_t n)` returns the sum of the diagonal `m[0][0] + m[1][1] + ...` of an `n` by `n` matrix of 32-bit `int`s stored row by row. The elements are signed; the sum is a 64-bit `long`.

Arguments: pointer in `rdi`, `n` in `rsi`. For `n = 0` do not read or write memory (`trace` returns 0). Preserve `rbx`, `rbp` and `r12`-`r15`.
