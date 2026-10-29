Use a lookup string for encoding: `static const char D[] = "0123456789abcdef";` then `out[2*i] = D[in[i] >> 4];` and `out[2*i+1] = D[in[i] & 15];`. Do not forget the final `'\0'`.
===
For decoding, write a small helper `static int nibble(char c)` that returns 0 to 15, or -1 if `c` is not a hex digit. Handle `'0'..'9'`, `'a'..'f'` and `'A'..'F'` with ranges: `c - '0'`, `c - 'a' + 10`.
===
Find the length with `strlen`. If it is odd return -1. Then for each pair take `hi = nibble(hex[2*i])`, `lo = nibble(hex[2*i+1])`, return -1 if either is negative, and store `(uint8_t)((hi << 4) | lo)`.
