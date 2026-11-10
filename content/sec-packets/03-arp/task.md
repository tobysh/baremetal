Keep the supplied `ARP` struct. Implement `int parse_arp(const uint8_t *p, size_t n, ARP *out)` for an Ethernet/IPv4 ARP message whose Ethernet header has already been removed.

**Return value:** 0 for a complete supported message, -1 otherwise. On failure `*out` is unchanged.

**Accept only when all of these hold:**

| Bytes | Field | Required value |
| --- | --- | --- |
| 0..1 | hardware type | 1 (Ethernet) |
| 2..3 | protocol type | 0x0800 (IPv4) |
| 4 | hardware length | 6 |
| 5 | protocol length | 4 |
| 6..7 | operation | 1 (request) or 2 (reply) |

`n` must be at least 28; trailing bytes (link padding) are allowed and ignored. NULL `p` or `out` returns -1.

**On success fill:**

- `opcode`: the operation (big-endian, bytes 6..7).
- `sender`: the IPv4 at bytes 14..17 as a host-order number, so 192.0.2.10 becomes `0xc000020a` on every machine.
- `target`: the IPv4 at bytes 24..27, built the same way.

The MAC fields at 8..13 and 18..23 are not returned. The tests use many addresses, including ones whose first byte is 128 or more, and try every format field and the 27/28 byte boundary.
