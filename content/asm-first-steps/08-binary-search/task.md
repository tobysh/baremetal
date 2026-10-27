Implement `long binary_search(const long *a, size_t n, long key)` in assembly. The array `a` of `n` signed `long`s is **sorted in ascending order**. Return an index `i` with `a[i] == key`, or `-1` if the key is not present. (If the key appears several times, any matching index is accepted.)

Arguments: `a` in `rdi`, `n` in `rsi`, `key` in `rdx`. Return in `rax`. It must work for `n = 0`, must not read outside the array, and must leave `rbx`, `rbp` and `r12`-`r15` unchanged.
