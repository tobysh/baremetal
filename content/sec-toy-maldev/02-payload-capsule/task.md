The capsule format is invented for this course; it is distinct from the analysis course's BMX files.

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | ASCII magic `BML1` |
| 4 | 2 | entry **byte offset** into code, little-endian uint16 |
| 6 | 2 | code byte length, little-endian uint16 |
| 8 | 4 | IEEE CRC32 of code bytes, little-endian uint32 |
| 12 | length | code bytes |

Code length is 1–4096; entry must be smaller than length. No trailing bytes are allowed. CRC32 detects accidental corruption; an attacker can recompute it, so it does not establish authenticity.

Implement `Wrap(code []byte, entry uint16) ([]byte, error)`. Validate only the length and numeric entry range here; instruction decoding and entry-boundary validation belong to the loader. Return nil,error for invalid ranges. Do not modify code and do not retain aliases into its storage; the returned capsule must be independent. Larger instruction streams from lesson 1 must be reduced to fit the container's 4096-byte limit.

Example: code `[0]`, entry 0 yields a 13-byte capsule. The checksum covers the one HALT byte, not the header.

