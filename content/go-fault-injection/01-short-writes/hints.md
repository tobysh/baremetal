Look at what `Write` returns: `n` can be smaller than `len(p)` with a nil error. One call is not enough; you need a loop over the remaining slice.
===
Keep `total` and re-slice `p = p[n:]` after each call. Count `stalls` for consecutive zero-byte results and reset it when `n > 0`. Check `err` before looking at `n`, but still add `n` to the total.
===
`ReadFrame`: `io.ReadFull(r, hdr[:])` returns `io.EOF` only if no bytes were read, and `io.ErrUnexpectedEOF` for a partial header. Pass those through as they are.
===
For the body, `io.ReadFull` may return `io.EOF` when the body is entirely missing. The header was already read, so that case must become `io.ErrUnexpectedEOF`. A zero-length body returns no error from `io.ReadFull`.
