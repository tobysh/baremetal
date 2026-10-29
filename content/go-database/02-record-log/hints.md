`AppendRecord`: `rec := make([]byte, 8+len(key)+len(value))`, fill the two lengths with `binary.BigEndian.PutUint32`, `copy` the key at `rec[8:]` and the value after it, then `_, err := w.Write(rec)`.
===
`ReadRecord`: `var hdr [8]byte; _, err := io.ReadFull(r, hdr[:])`. If `err` is not nil, return it as it is: `io.ReadFull` already gives `io.EOF` for zero bytes and `io.ErrUnexpectedEOF` for a partial header.
===
Read the lengths with `binary.BigEndian.Uint32`, check them against `MaxField` **before** `make`, then `io.ReadFull` into one body buffer of `kl+vl` bytes.
===
Once the header has been read, running out of bytes is always a cut-off record. `io.ReadFull` returns `io.EOF` if the body read got zero bytes, so turn `io.EOF` into `io.ErrUnexpectedEOF` there.
