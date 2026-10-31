The codec from lesson 1 is given. Implement:

- `ReadFrame(r io.Reader) (Frame, error)`: returns exactly one frame and consumes exactly its bytes (or, on a bad header, at most the 4 header bytes). Errors: `io.EOF` (nothing read), `io.ErrUnexpectedEOF` (cut mid-frame), `ErrMagic`, `ErrTooLarge`, `ErrChecksum`, or the reader's own error.
- `WriteFrame(w io.Writer, typ byte, payload []byte) error`: encodes and issues a single `Write`. An oversize payload returns `ErrTooLarge` and writes nothing. A writer error is returned (`errors.Is` must match). A short write with a nil error becomes `io.ErrShortWrite`.

Hidden tests use readers that return one byte at a time, random pieces, `(n>0, io.EOF)` together, and `(0, nil)`, and count how many bytes you consumed.
