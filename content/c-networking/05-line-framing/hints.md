`lb_feed`: check `lb->len + n > sizeof lb->buf` first. Otherwise `memcpy(lb->buf + lb->len, data, n); lb->len += n;`.
===
`lb_next`: use `memchr(lb->buf, '\n', lb->len)` to find the newline. If it is NULL, return -1. Otherwise the line is `buf[0 .. pos)`; if its last byte is `'\r'`, drop that too. After copying, `memmove` the rest of the buffer down over the consumed `pos + 1` bytes and reduce `len`.
===
`parse_command`: skip leading spaces, copy letters up to the next space with `toupper` (watch the 15 limit), then skip spaces and set `*arg = p`. An empty verb means return -1.
