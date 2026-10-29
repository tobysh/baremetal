Use the string instructions to implement:

- `void *fill_bytes(void *dst, int c, size_t n)` sets `n` bytes to the low byte of `c` and returns `dst`.
- `void *move_bytes(void *dst, const void *src, size_t n)` copies `n` bytes and works correctly when the buffers overlap (like `memmove`). Returns `dst`. The direction flag must be clear when it returns.
- `size_t str_length(const char *s)` returns the length of a NUL-terminated string using `repne scasb`.
