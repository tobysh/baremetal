Loop while `len(buf) > 0`: `Decode(buf)`. Success: emit, `buf = buf[n:]`, `skipping = false`. `ErrShort`: break (unless closing, then treat as `ErrTruncated`).
===
On a failure: if `!skipping`, append the error and set `skipping = true`. Then `i := bytes.IndexByte(buf[1:], Magic)`. If `i < 0` discard all of `buf`; otherwise discard `i+1` bytes. Add the discarded count to `dropped`.
===
Searching from `buf[1:]` is what guarantees progress and what lets you find a real frame hidden inside a false one. Without it a lone `0xBE` makes you loop forever.
