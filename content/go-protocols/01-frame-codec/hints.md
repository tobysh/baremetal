Build the header with `[]byte{Magic, typ, byte(n>>8), byte(n)}`, append the payload, then `crc32.ChecksumIEEE(b)` over everything so far.
===
The order of checks in `Decode` matters: empty -> `ErrShort`; `b[0] != Magic` -> `ErrMagic`; fewer than 4 bytes -> `ErrShort`; length too big -> `ErrTooLarge`; fewer than the total -> `ErrShort`; then the CRC.
===
`total := HeaderLen + l + TrailerLen`. The CRC is over `b[:total-4]` and is stored big endian in `b[total-4:total]`. Compare as `uint32` (`binary.BigEndian.Uint32`).
