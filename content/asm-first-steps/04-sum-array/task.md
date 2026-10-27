Implement `long sum_array(const long *a, size_t n)` in assembly. It returns the sum of the `n` elements of `a`, or `0` if `n` is `0`.

`a` arrives in `rdi` and `n` in `rsi`. Each element is 8 bytes. Return the total in `rax`. The array is read-only, and for `n = 0` you must not touch memory at all.
