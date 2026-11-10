Keep the supplied `UDP` struct. Implement `int parse_udp(const uint8_t *p, size_t n, UDP *out)`. The span `n` is the transport payload supplied by the caller: it begins at the UDP header and may include trailing bytes that are not part of the datagram.

**Return value:** 0 on success, -1 on a NULL pointer, `n < 8`, or an invalid declared length. On failure `*out` is unchanged.

**Fields (big-endian, two bytes each):**

| Bytes | Field | Use |
| --- | --- | --- |
| 0..1 | source port | `src` |
| 2..3 | destination port | `dst` |
| 4..5 | length | header plus payload |
| 6..7 | checksum | ignored |

**Rules:**

- Require `8 <= length <= n`. A length of 0 to 7, or one larger than `n`, is invalid.
- `payload = length - 8`. It is derived from the declared length, never from `n`; bytes after `length` are ignored.
- Do not validate the checksum.

The tests vary the lengths from 8 up to 65535, add trailing bytes, and try each side of both bounds (`length` 7, 8, `n` and `n + 1`). Keep the supplied function name and types.
