Write a small `be16` and `be32` helper first. Every field in ARP is big-endian, so the helpers carry the whole parser.
===
Validate in this order: NULL pointers and `n >= 28`, then hardware type, protocol type, hardware length and protocol length, then the operation. Only after that read the addresses at offsets 14 and 24.
===
In `be32`, cast each byte to `uint32_t` before shifting. `p[0] << 24` is computed as `int` and is undefined for a first byte of 128 or more, such as 198.51.100.7.
===
Assign `*out` once, at the end, from a local value. That keeps a failed parse from leaving a half-written struct behind.
