The serial helpers (`serial_init`, `serial_putc`), the `KbdState`, `KEY_*` constants and tables, `kbd_feed`, and the `LineEditor` struct (`buf[LINE_MAX]`, `len`, `done`) are in the starter. Implement:

- `void line_init(LineEditor *e)` makes the editor empty (no text, not done).
- `int line_feed_char(LineEditor *e, int ch)` applies the table in the lesson to one decoded character and echoes through `serial_putc`. If `e->done` is set, it first starts a fresh empty line. It returns 1 only when `ch` is `'\n'` (setting `e->done`), otherwise 0. Characters outside the table (including values above 255) are ignored.
- `int line_feed_scancode(LineEditor *e, KbdState *k, uint8_t sc)` decodes `sc` with `kbd_feed`; a result of 0 or above 255 is ignored (return 0), anything else goes to `line_feed_char`.
- `const char *line_text(LineEditor *e)` returns the current text as a NUL-terminated string (the buffer is terminated after every change).
- `size_t line_take(LineEditor *e, char *out, size_t cap)` copies the text into `out`, at most `cap - 1` characters followed by `'\0'` (nothing at all if `cap` is 0), starts a new empty line and returns the full length of the line that was in the editor.
