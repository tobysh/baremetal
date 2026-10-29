The `LineBuf` struct (a 64 byte buffer and a length) is in the starter. Implement:

- `void lb_init(LineBuf *lb)` empties the buffer.
- `int lb_feed(LineBuf *lb, const char *data, size_t n)` appends `n` bytes. If they do not all fit, append nothing and return `-1`; otherwise return `0`.
- `int lb_next(LineBuf *lb, char *out, size_t cap)` looks for a complete line (ending in `\n`). If there is one, copy it into `out` **without** the `\n` and without a `\r` just before it, add a terminating `'\0'`, remove it (and its newline) from the buffer, and return its length. Return `-1` if there is no complete line. A line is never longer than 63 bytes and `cap` is at least 64.
- `int parse_command(const char *line, char verb[16], const char **arg)` splits a line: `verb` receives the first word **upper-cased** (NUL-terminated), and `*arg` points into `line` at the first character after the spaces that follow the verb (an empty string if there is none). Return `0`. Return `-1` for an empty or all-space line, or if the verb is longer than 15 characters.

Empty lines (just `\n`) are real lines of length 0. Other inputs are tried.
