Implement the frame format described above (the constants, errors and `Frame` type are already in the starter).

- `Encode(typ byte, payload []byte) ([]byte, error)` returns the encoded frame, or `ErrTooLarge` if the payload exceeds `MaxPayload`.
- `Decode(b []byte) (typ byte, payload []byte, n int, err error)` decodes **one** frame from the start of `b` and reports `n`, the bytes it used (bytes after the frame are left alone). On any error return `n == 0`.
  - empty input or a valid prefix: `ErrShort`
  - first byte is not `Magic`: `ErrMagic` (even if it is the only byte)
  - length above `MaxPayload` once 4 header bytes are present: `ErrTooLarge`
  - CRC mismatch: `ErrChecksum`
  - the returned payload must be a copy, not a view of `b`.

Hidden tests build frames with their own reference encoder, cut them at every length, flip every single bit, and check the golden bytes of a known frame. Use `hash/crc32`.
