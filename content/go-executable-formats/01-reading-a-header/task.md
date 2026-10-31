Implement three functions in `starter.go`:

- `ReadU16(b, off)` and `ReadU32(b, off)` read a little-endian value at byte offset `off` (a `uint64`, so hostile offsets are representable). If the value does not fit completely inside `b`, return an error wrapping `ErrTruncated`. Offsets such as `1<<64 - 1` must fail cleanly.
- `ParseHeader(b)` validates the 16-byte header (layout in the starter) and fills `Header`. Checks in this order: fewer than 16 bytes is `ErrTruncated`; wrong magic `ErrMagic`; version other than 1 `ErrVersion`; non-zero flags `ErrCorrupt`; finally the tables (sections 16 bytes each, symbols 12, relocations 16, then the string table) must all end inside the input, otherwise `ErrTruncated`.

`Header.SecOff` is 16 and each following offset adds the previous table's size; `End` is the first byte after the string table. Run uses a tiny example. The hidden tests cut and damage inputs at every length and use counts and sizes at their maximum.
