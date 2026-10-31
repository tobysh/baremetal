Implement `BitWriter` and `BitReader` in `starter.go`.

- `WriteBits(v, n)` appends the low `n` bits of `v` (`0 <= n <= 64`), highest of those bits first. Bits of `v` above `n` are ignored.
- `Len()` is the number of bits written. `Bytes()` returns a **copy** of the data, the last byte padded with zero bits; the writer must keep working after `Bytes()`.
- `NewBitReader(data)`, `ReadBits(n)` returns the next `n` bits as the low bits of the result. With fewer than `n` bits left it returns `ErrShort` and leaves the position unchanged. `ReadBits(0)` always succeeds.
- `Pos()` is bits consumed, `Remaining()` is bits left (including padding bits, since the reader cannot tell them apart).

The hidden tests check known byte patterns, compare random mixed-width writes against a string-of-`'0'`/`'1'` reference, and read everything back.