Implement in `starter.go`:

- `AppendUvarint(dst, v)` appends the canonical varint of `v` to `dst` and returns it.
- `Uvarint(b)` returns `(value, bytesUsed, err)`. Extra bytes after the varint are left alone. On any error it returns `0, 0, err`, with `ErrShort` (input ended), `ErrOverflow` or `ErrNonCanonical`. Overflow is reported as soon as it is certain, even if the input also ends.
- `AppendVarint` / `Varint`: the zigzag versions for `int64`; decoding errors pass through.

Hidden tests include fixed vectors (`300` is `ac 02`, the largest `uint64` is nine `ff` then `01`), a round trip on thousands of values, and every proper prefix of an encoding must give `ErrShort`.