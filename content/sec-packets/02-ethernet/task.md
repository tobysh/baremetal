Keep the supplied `Ethernet` struct. Implement `int parse_ethernet(const uint8_t *p, size_t n, Ethernet *out)`.

**Return value:** 0 on success, -1 if `p` or `out` is NULL or the header is truncated. On failure `*out` must not be modified.

**Fields to fill:**

| Field | Source |
| --- | --- |
| `dst` | bytes 0..5, copied as-is |
| `src` | bytes 6..11, copied as-is |
| `type` | bytes 12..13, big-endian |
| `payload` | offset of the first payload byte |

**Rules:**

- Require `n >= 14`. A frame of exactly 14 bytes succeeds with `payload == 14`.
- If the EtherType at bytes 12..13 is `0x8100`, require `n >= 18`. Then `type` is the big-endian value at bytes 16..17 (the encapsulated type) and `payload == 18`. Bytes 14..15 (the tag control information) are ignored.
- Any other EtherType, including `0x8100` appearing again as the inner type, is stored as given. Only one tag is unwrapped.
- Trailing bytes beyond the header are not examined. Do not validate the payload protocol.

The tests build frames with many different MACs, types and lengths, and try each boundary length. Keep the supplied function name and types.
