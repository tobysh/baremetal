Implement:

- `void map_inplace(long *a, size_t n, long (*f)(long))` replaces every `a[i]` by `f(a[i])`, in order.
- `long fold(const long *a, size_t n, long init, long (*f)(long acc, long x))` starts with `acc = init`, and for each element in order sets `acc = f(acc, a[i])`; it returns `acc`. With `n = 0` it returns `init`.

The callbacks may destroy all caller-saved registers. You must preserve `rbx`, `rbp`, `r12`-`r15`, and keep the stack 16-byte aligned at each `call`.
