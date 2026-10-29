The oldest item lives at `buf[head]`. The next free slot is `(head + len) % cap`.
===
`q_push`: if `len == cap` return -1; write at `(head + len) % cap`; `len++`. `q_pop`: if `len == 0` return -1; read `buf[head]`; `head = (head + 1) % cap`; `len--`.
===
`q_init` must set all four fields (`buf`, `cap`, `head = 0`, `len = 0`). If the allocation fails or `cap <= 0`, set `buf = NULL` and return -1. In `q_free`, call `free(q->buf)` and reset the fields.
