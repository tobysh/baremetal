Implement `void reverse_array(long *a, size_t n)` in assembly. It reverses the `n` elements of `a` **in place**: afterwards `a[0]` holds what used to be `a[n-1]`, and so on.

`a` arrives in `rdi` and `n` in `rsi`. There is no return value. It must work for `n = 0`, `n = 1`, and both even and odd lengths, and must not touch memory outside `a[0..n-1]`.
