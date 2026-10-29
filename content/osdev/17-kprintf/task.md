Implement `int ksnprintf(char *buf, size_t size, const char *fmt, ...)`. `<stdarg.h>` is included in the starter.

- Supports `%c`, `%s`, `%d`, `%u`, `%x`, `%p`, `%%`, the `l` length modifier for `d`, `u` and `x`, the `0` flag and a decimal width (see the lesson for the exact rules). An unknown conversion is copied unchanged, including its `%`.
- Writes at most `size - 1` characters followed by `'\0'`, and writes nothing when `size` is 0 (so `buf` may be `NULL` then).
- Returns the length the whole output would have had, not counting the `'\0'`.
- `%s` of `NULL` prints `(null)`. `%p` prints `0x` and the value in lower-case hex (`0x0` for a null pointer).

Also implement `int kprintf(const char *fmt, ...)`, which formats into a 256-byte buffer with the same rules and sends the result, up to the end of that buffer, to `serial_putc` (a provided function). It returns the length of the *complete* output, as `ksnprintf` would.
