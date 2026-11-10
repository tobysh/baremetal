Keep `pos` as the offset of the next header and `rem = n - pos` as what is left. Every decision is made from `rem`: 0 means a clean end, under 5 means a partial header.
===
Validate the five header bytes before looking at the body: type in 20..23, `b[pos+1] == 3`, `b[pos+2]` in 1..4, and `((size_t)b[pos+3] << 8 | b[pos+4])` in 1..16384. A failure returns -1 even if the body is missing.
===
Then compare `length` with `rem - 5` (no addition, so no overflow). If the body is not all there, return 1. Otherwise advance `pos` by `5 + length` and increment the record count.
===
Set `*records` and `*consumed` on every exit path (guarding for NULL), and set them to the complete records only. The NULL-buffer case with `n > 0` returns -1 with both outputs 0.
