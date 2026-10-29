`Frame`: allocate `8+len(payload)`. Write the length at `[4:8]`, copy the payload to `[8:]`, then compute the CRC over `f[4:]` (that is exactly the length bytes plus the payload) and put it at `[0:4]`.
===
`crc32.ChecksumIEEE(f[4:])` gives the same number as chaining `crc32.Update` over the two pieces, so you can verify in `Unframe` with the same slice expression.
===
`Unframe` order: fewer than 8 bytes returns `ErrShort`; read `n := uint64(len)`; `if uint64(len(data)-8) < n` returns `ErrShort` (use `uint64` so a huge length cannot overflow); only then compare checksums.
===
`ScanFrames`: loop while `valid < len(data)`; call `Unframe(data[valid:])`. On `ErrShort` break with a nil error. On `ErrCorrupt` return `fmt.Errorf("offset %d: %w", valid, err)` so `errors.Is` works.
