Implement:

- `size_t u64_to_dec(uint64_t v, char *buf)` writes the decimal digits of `v` followed by a NUL byte into `buf` and returns the number of digits (not counting the NUL). `0` becomes `"0"`. The buffer has room for 21 bytes, and nothing past the NUL may be written.
- `long parse_dec(const char *s)` parses an optional `+` or `-` sign followed by decimal digits and stops at the first character that is not a digit. No leading spaces. If there are no digits it returns 0. Results always fit in a `long`.
