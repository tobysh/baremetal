Build helper `appendChunk(out, raw, enc)`: two uvarints, `enc`, then `binary.BigEndian.AppendUint32(out, crc32.ChecksumIEEE(raw))`.
===
Reader loop with a position `pos` into the body after the 4 header bytes. Use `Uvarint(body[pos:])`; `ErrShort` -> `ErrTruncated`, other errors -> `ErrCorrupt`.
===
Compare sizes as `uint64` against the remaining budget (`uint64(maxOutput - len(out))`) so a huge claimed length cannot overflow `int`.
===
Call `Unpack(enc, Limits{MaxOutput: int(rawLen)})` and map every error to `ErrCorrupt` (rawLen was already checked against the budget, so `ErrTooLarge` here means the chunk lies).
===
Check the CRC only after the length matches, and check there are 4 bytes left before reading it.
