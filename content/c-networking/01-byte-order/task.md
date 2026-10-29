Implement these five functions. All multi-byte values on the wire are **big-endian**.

- `void put_u16(uint8_t *p, uint16_t v)` writes 2 bytes at `p`.
- `void put_u32(uint8_t *p, uint32_t v)` writes 4 bytes at `p`.
- `uint16_t get_u16(const uint8_t *p)` reads 2 bytes from `p`.
- `uint32_t get_u32(const uint8_t *p)` reads 4 bytes from `p`.
- `uint32_t swap32(uint32_t v)` reverses the four bytes of a value (`0x11223344` becomes `0x44332211`).

Write exactly the bytes asked for, no more. Do not use `htons`/`htonl`; use shifts. Many different values are tried.
