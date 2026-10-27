Implement two functions in assembly:

- `long sum_clamped(const long *a, size_t n, long lo, long hi)` returns the sum of the `n` elements of `a`, where each element is first forced into the range `lo..hi` (values below `lo` count as `lo`, values above `hi` count as `hi`). `lo` is never greater than `hi`. For `n = 0` the answer is `0` and the array must not be read.
- `size_t count_negative(const long *a, size_t n)` returns how many of the `n` elements are less than zero (signed). For `n = 0` the answer is `0`.

Arguments follow the System V convention: `a` in `rdi`, `n` in `rsi`, then `lo` in `rdx` and `hi` in `rcx` for the first function. Neither function may modify the array.
