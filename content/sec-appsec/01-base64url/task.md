Implement `int b64u_decode(const char *in, size_t n, uint8_t *out, size_t cap, size_t *len)`. Decode the `n` characters of `in` (not NUL terminated) into `out`, set `*len` to the number of bytes and return 0. Return -1 (and write nothing past `cap` bytes) if a character is outside `A-Za-z0-9-_`, if `n % 4 == 1`, if the decoded data would need more than `cap` bytes, or if the leftover bits after the last byte are not zero. An empty input decodes to zero bytes.

Tests use random data and many malformed strings. Keep the supplied function names and types.
