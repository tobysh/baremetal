Write `ReadU32` first. Return early with `fmt.Errorf("...: %w", ErrTruncated)` when `off > len(b)` or `len(b)-off < 4`; both sides are `uint64`.
===
`ReadU16` is the same with 2. You may use `ReadU16`/`ReadU32` inside `ParseHeader` or index the slice directly after the 16-byte length check; both are fine.
===
Do the length check (`len(b) < 16`) before looking at the magic; a 4-byte file with the wrong magic is a truncation, not a magic error.
===
Convert counts to `uint64` before multiplying: `16*uint64(h.NSec)`. With `NSec = 65535` and `StrSize = 0xFFFFFFFF` the sum exceeds 32 bits, so keep everything in `uint64` until the final comparison with `uint64(len(b))`.
