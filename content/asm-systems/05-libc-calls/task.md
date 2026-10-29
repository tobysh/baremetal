Implement two functions that call libc:

- `int format_pair(char *buf, size_t n, long a, long b)` writes `"<a>,<b>"` (for example `"-4,700"`) into `buf` using `snprintf` with the format `"%ld,%ld"`, and returns `snprintf`'s result (the length the full text would have, as in C).
- `size_t joined_length(const char *a, const char *b)` returns `strlen(a) + strlen(b)` by calling `strlen` twice.

Arguments follow the standard order (`rdi`, `rsi`, `rdx`, `rcx`). Keep the stack 16-byte aligned at every `call`, set `al` before the variadic call, and preserve `rbx`, `rbp` and `r12`-`r15`.
