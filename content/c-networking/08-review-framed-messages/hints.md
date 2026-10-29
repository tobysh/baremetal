`send_frame`: build `uint8_t out[2 + MAX_FRAME]`, `put_u16(out, n)`, `memcpy(out + 2, payload, n)`, then one `write_full(fd, out, 2 + n)`.
===
`recv_frame`: `r = read_full(fd, hdr, 2)`. `r == 0` is a clean close (-2), `r == 1` is a truncated header (-1). Decode the length, reject if it is above `MAX_FRAME` or above `cap`, then `read_full` the payload and require that you got all of it.
===
`serve_reversed`: loop `n = recv_frame(...)`; if `n == -2` return the count; if `n < 0` return -1; reverse the bytes in place with two indices; `send_frame` them; count++.
