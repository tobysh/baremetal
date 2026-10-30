Implement three functions in the starter.

- `WriteFull(w io.Writer, p []byte) (int, error)` writes all of `p`, continuing after short writes. It returns the number of bytes accepted. On a write error it returns the bytes accepted so far and that error unchanged. After 3 consecutive calls that accept 0 bytes with no error it returns the accepted count and `io.ErrShortWrite`. An empty `p` makes no `Write` call.
- `WriteFrame(w io.Writer, payload []byte) error` writes a 2-byte big-endian length then the payload, through `WriteFull`. Payloads over 65535 bytes return `ErrTooLarge` and write nothing. Build the whole frame first and write it with one `WriteFull`.
- `ReadFrame(r io.Reader) ([]byte, error)` reads one frame. A clean end before any header byte returns `io.EOF`. An end anywhere inside a frame returns `io.ErrUnexpectedEOF`, including a complete header with a missing body. A zero-length frame returns an empty non-error result.

The tests feed your code many failure schedules, including one-byte-at-a-time readers, and cut a valid stream at every possible byte.
