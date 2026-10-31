Implement `StreamDecoder` (`NewStreamDecoder`, `Write`, `Close`) in `starter.go`. The starter provides `parseHeader`, `buildTable` and `lookup` from the previous lesson, so you do not rewrite header validation.

- Until the header is complete, buffer bytes and call `parseHeader` after each write. `ErrTruncated` from it means "wait". Any other error is final.
- Once the header parses, process the rest of that write (and all later bytes) as payload, bit by bit, returning the symbols decoded in this call.
- When all `n` symbols are decoded, the remaining bits of the current byte must be zero, and any further byte is `ErrCorrupt`.
- Errors are sticky. `Close` returns the sticky error if there is one, `ErrTruncated` if the stream is incomplete, otherwise nil.

Hidden tests: every chunking pattern, a split at every position, per-byte promptness, every truncation, trailing data, and 1500 damaged streams against the independent reference (accept or reject must match).