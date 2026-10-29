Implement `size_t count_matching(const long *a, size_t n, long (*pred)(long))`. For every element of `a` call `pred(a[i])`; count the elements for which it returns a non-zero value and return that count.

`a` is in `rdi`, `n` in `rsi`, `pred` in `rdx`; the result goes in `rax`. `pred` may overwrite every caller-saved register, so keep your loop state in callee-saved registers, and the stack must be 16-byte aligned at each `call`. Your function must leave `rbx`, `rbp` and `r12`-`r15` exactly as it found them. `n` may be 0.
