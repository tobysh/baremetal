Keep the supplied `TCP` struct. Implement `int parse_tcp(const uint8_t *p, size_t n, TCP *out)`. The span `n` is exactly the TCP segment (header plus payload): the caller already removed the Ethernet and IP headers and any padding.

**Return value:** 0 on success, -1 on a NULL pointer, `n < 20`, or an invalid data offset. On failure `*out` is unchanged.

**Header length:** `header = (p[12] >> 4) * 4`. It must satisfy `20 <= header <= n`. Offsets 0 to 4 and any header longer than `n` are errors. The low nibble of byte 12 (reserved and NS bits) is ignored.

**Fill on success (all multi-byte fields are big-endian):**

| Field | Source |
| --- | --- |
| `src`, `dst` | bytes 0..1 and 2..3 |
| `seq`, `ack` | bytes 4..7 and 8..11 as 32-bit values, with no sign changes |
| `flags` | byte 13, unmodified |
| `header` | the header length in bytes |
| `payload` | `n - header` |

Do not validate checksums or option syntax. The tests try offsets from 5 to 15, ports 0 and 65535, sequence numbers near 2^32, odd flag bytes, and spans one byte short of the header. Keep the supplied function name and types.
