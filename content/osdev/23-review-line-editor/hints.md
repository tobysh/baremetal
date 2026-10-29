Keep the buffer always terminated: after any change write `e->buf[e->len] = '\0';`. Then `line_text` is just `return e->buf;`, and `line_init` is `e->len = 0; e->done = 0; e->buf[0] = 0;`.
===
Put a tiny helper `static void erase_one(void) { serial_putc('\b'); serial_putc(' '); serial_putc('\b'); }` at the top and reuse it for both Backspace and `Ctrl+U` (loop `while (e->len > 0) { e->len--; erase_one(); }`).
===
In `line_feed_char`, begin with `if (e->done) { e->len = 0; e->done = 0; }`, then a chain: `'\n'` finishes (echo `\r` then `\n`), `'\b'` (and `127`, if you like) deletes, `21` clears, `3` clears with `"^C\r\n"`, and `ch >= 32 && ch <= 126` appends only `if (e->len < LINE_MAX - 1)`.
===
`line_feed_scancode`: `int c = kbd_feed(k, sc); if (c <= 0 || c > 255) return 0; return line_feed_char(e, c);`.
===
`line_take`: remember `size_t n = e->len;`, copy `min(n, cap - 1)` bytes when `cap > 0`, terminate `out`, then call `line_init(e)` and `return n;`.
