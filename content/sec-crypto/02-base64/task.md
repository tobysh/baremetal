Write `size_t b64_encode(const uint8_t *in, size_t n, char *out)`. It writes the standard base64 text (alphabet `A-Za-z0-9+/`, padded with `=`) to `out`, adds a `'\0'`, and returns the number of characters before the `'\0'`.

The output length is always `4 * ceil(n / 3)`. The empty input gives the empty string. The tests try binary data of many lengths, including lengths that need `==` and `=`.
