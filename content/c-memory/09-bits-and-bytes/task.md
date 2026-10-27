Write three functions on 32-bit unsigned values:

- `int count_bits(uint32_t x)` returns how many bits of `x` are 1. `count_bits(0)` is 0 and `count_bits(0xFFFFFFFF)` is 32.
- `int is_power_of_two(uint32_t x)` returns 1 if `x` is 1, 2, 4, 8, ... and 0 otherwise. Zero is not a power of two.
- `uint32_t swap_endian(uint32_t x)` reverses the order of the four bytes: `0x12345678` becomes `0x78563412`.
