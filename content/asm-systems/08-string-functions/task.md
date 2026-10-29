Implement three functions with libc semantics (do not call libc):

- `size_t my_strlen(const char *s)` returns the number of bytes before the NUL.
- `void *my_memcpy(void *dst, const void *src, size_t n)` copies `n` bytes and returns `dst`. The regions do not overlap. `n` may be 0.
- `int my_strcmp(const char *a, const char *b)` returns a negative, zero or positive `int` as `a` is less than, equal to or greater than `b`, comparing bytes as **unsigned** values. Only the sign matters.

The hidden test places strings flush against an inaccessible page, so never read past a terminator or copy more than `n` bytes. Preserve `rbx`, `rbp` and `r12`-`r15`.
