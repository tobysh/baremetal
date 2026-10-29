Implement two functions with SSE packed instructions:

- `void add_arrays(float *dst, const float *a, const float *b, size_t n)` sets `dst[i] = a[i] + b[i]` for `i < n`. Pointers need not be aligned, `n` is arbitrary (including 0 and values that are not multiples of 4).
- `float dot_product(const float *a, const float *b, size_t n)` returns the sum of `a[i] * b[i]`, or `0.0f` for `n = 0`.

Arguments: pointers in `rdi`, `rsi`, `rdx`, `rcx` and so on, `n` after them; `dot_product` returns a float in `xmm0`. Process four elements per iteration, then handle the remainder one element at a time. Never access memory beyond `n` elements. Preserve `rbx`, `rbp` and `r12`-`r15`.
