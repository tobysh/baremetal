The `Header` struct and the byte-order helpers are in the starter. Implement:

- `void hdr_encode(const Header *h, uint8_t out[10])` writes the 10 byte wire form described in the lesson. Only the low 4 bits of `version` and `flags` are used.
- `int hdr_decode(const uint8_t *buf, size_t n, Header *h)` reads a header from the first bytes of `buf`, where `n` bytes are available.
  - If `n < 10`, return `0` ("need more data") and do not touch `h`.
  - If the magic is not `0xB4DE`, return `-1` and do not touch `h`.
  - Otherwise fill in `h` and return `10`, the number of bytes consumed. Extra bytes after the header are ignored.

Never read past `n` bytes. Many different headers are tried.
