Keep the supplied `IPv4` struct. Implement `int parse_ipv4(const uint8_t *p, size_t n, IPv4 *out)`.

**Return value:** 0 on success, -1 on failure. On failure `*out` is unchanged. NULL `p` or `out` returns -1.

**Validation, in terms of `n` (bytes supplied):**

1. `n >= 20`.
2. Version (high nibble of byte 0) is 4.
3. Header length `h = (p[0] & 15) * 4` satisfies `20 <= h <= n`.
4. Total length (bytes 2..3, big-endian) satisfies `h <= total <= n`.

Bytes after `total` are Ethernet padding: accept them and ignore them. Do not validate the header checksum or option contents.

**On success fill:**

| Field | Value |
| --- | --- |
| `total` | bytes 2..3, big-endian |
| `protocol` | byte 9 |
| `src` | bytes 12..15 as a host-order number (192.0.2.10 is `0xc000020a`) |
| `dst` | bytes 16..19, same encoding |
| `header` | `h`, in bytes |
| `fragmented` | 1 if `(flags_and_offset & 0x3fff) != 0`, else 0 |

`flags_and_offset` is the big-endian 16-bit value at bytes 6..7. The Don't Fragment bit (0x4000) and the reserved bit (0x8000) do not count as fragmentation; More Fragments (0x2000) and any nonzero offset do.

The tests vary IHL from 5 to 15, total length against `n`, the flag bits and the addresses. Keep the supplied function name and types.
