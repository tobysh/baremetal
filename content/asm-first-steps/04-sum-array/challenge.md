**Challenge: the largest element.**

Write `max_array` in assembly:

```c
long max_array(const long *a, size_t n);   // a in rdi, n in rsi
```

Return the largest of the `n` signed 64-bit values at `a`. `n` is always at least 1. Negative numbers are common, so compare as **signed** (`jg`/`jl`, not `ja`/`jb`), and do not start from 0: start from the first element.

You may only use caller-saved registers (`rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`-`r11`), so there is nothing to save and restore.
