Implement `Parser` (the codec is given above it):

- `NewParser() *Parser`
- `(*Parser).Feed(chunk []byte) ([]Frame, error)` appends `chunk` and returns every frame that is now complete, in order. Empty and nil chunks are fine.
- On a bad frame, return the frames completed *before* it together with the error (`ErrMagic`, `ErrTooLarge` or `ErrChecksum`). Every later `Feed` returns `nil` and the same error.
- `(*Parser).Buffered() int` is the number of bytes held waiting for more input (0 after an error).

Hidden tests feed one stream whole, a byte at a time, split at every single position, and in hundreds of seeded random chunkings, from a buffer the test overwrites between calls.
