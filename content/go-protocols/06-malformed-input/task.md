The codec and the error values are given. Implement `Scanner`:

- `NewScanner() *Scanner`
- `Feed(chunk []byte) ([]Frame, []error)` appends the chunk and returns the frames now complete plus the errors found. Errors are `ErrMagic`, `ErrTooLarge` or `ErrChecksum` (check with `errors.Is`).
- `Close() ([]Frame, []error)` finishes the stream: leftover partial data yields `ErrTruncated` (once per bad region) and resynchronisation continues until the buffer is empty.
- `Buffered() int` is the bytes waiting; `Dropped() int` the total bytes discarded.

Rules: on a failed decode, drop at least one byte, then drop everything before the next `Magic` byte (or everything, if there is none). Report an error only if the scanner is not already skipping; clear that flag when a frame decodes. When data is `ErrShort` during `Feed`, wait for more.
