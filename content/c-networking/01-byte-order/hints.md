`put_u16` stores the high byte at `p[0]` and the low byte at `p[1]`: `p[0] = v >> 8; p[1] = v & 0xff;`.
===
For `get_u32`, cast each byte to `uint32_t` first: `((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ...`.
===
`swap32` can be written as `put_u32` into a temporary 4-byte array followed by a little-endian read, or with four shifts and masks: `(v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24)`.
