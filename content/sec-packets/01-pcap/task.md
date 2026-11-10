Implement `int pcap_count(const uint8_t *p, size_t n)`. It walks a classic pcap file held in memory and returns the number of complete records.

**Return value:** the record count (0 or more), or `-1` for any unsupported, malformed or truncated input. The input is never modified.

**Global header (24 bytes, required):**

| Offset | Size | Rule |
| --- | --- | --- |
| 0 | 4 | `d4 c3 b2 a1` selects little-endian, `a1 b2 c3 d4` selects big-endian; anything else is `-1` (this includes the nanosecond magics) |
| 4 | 2 | version major, must be 2 |
| 6 | 2 | version minor, must be 4 |
| 16 | 4 | snaplen, must be nonzero |
| 20 | 4 | link type, must be 1 (Ethernet) |

Offsets 8..15 (timezone, sigfigs) are ignored. Every integer after the magic uses the byte order the magic selected.

**Each record (16-byte header plus data):** microseconds at +4 must be below 1,000,000; captured length at +8 must be no larger than snaplen and no larger than the original length at +12; and the captured bytes must fit in what remains of the buffer. Fewer than 16 bytes left after the last record is `-1`.

A file with a valid global header and no records returns 0. `p == NULL` or `n < 24` returns -1. Do not compute `offset + length` unchecked: compare against `n - offset` once you have shown `offset <= n`.

The tests build many files, big and little endian, with varying snaplens and lengths, and try boundary values on each rule. Keep the supplied function name and types.
