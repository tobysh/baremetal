Implement `parseHeader` and `HuffDecode` in `starter.go`. The starter already contains `HuffEncode`, `BuildLengths`, `CanonicalCodes`, the bit reader and the varint code from earlier lessons.

- `parseHeader(data)` returns the header and the number of bytes it used. It reports `ErrTruncated` when the header is incomplete and `ErrCorrupt` when it is complete but breaks a rule in the lesson.
- `HuffDecode(enc)` returns the original bytes, or `nil` and one of the two errors. It must never panic and must never allocate for a count the payload cannot support.

Hidden tests: fixed vectors, every proper prefix of valid streams (`ErrTruncated`), a table of invalid headers, nonzero padding, trailing bytes, a header claiming 2^40 symbols (allocation is measured), and 1500 randomly damaged streams compared with an independent string-based reference decoder (same accept/reject decision, same output).