Use `io.ReadFull(r, hdr[:])` with `var hdr [4]byte`. It returns `io.EOF` only if no bytes were read and `io.ErrUnexpectedEOF` for a partial read, which is exactly the distinction asked for in the header.
===
After the header: check `hdr[0] == Magic` and the length against `MaxPayload` before reading more. Then `rest := make([]byte, l+TrailerLen)` and `io.ReadFull` again; an `io.EOF` there is still mid-frame, so turn it into `io.ErrUnexpectedEOF`.
===
Join `hdr` and `rest` into one slice and call `Decode` to verify the checksum and copy the payload. For `WriteFrame`: `n, err := w.Write(b)`; if `err != nil` return it; if `n < len(b)` return `io.ErrShortWrite`.
