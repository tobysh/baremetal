Fields: `buf []byte` and `err error`. In `Feed`: if `p.err != nil` return it. Otherwise `p.buf = append(p.buf, chunk...)` (this copies, so you never keep the caller's slice).
===
Loop: `typ, payload, n, err := Decode(p.buf)`. On `ErrShort` break out of the loop. On another error latch it, set `p.buf = nil` and return what you collected so far.
===
After a success, `p.buf = p.buf[n:]`. Appending the next chunk later still works because `append` reuses the remaining capacity or reallocates.
