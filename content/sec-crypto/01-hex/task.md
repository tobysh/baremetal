Write two functions.

`void hex_encode(const uint8_t *in, size_t n, char *out)` writes `2 * n` lowercase hex digits to `out` followed by a `'\0'`.

`int hex_decode(const char *hex, uint8_t *out)` converts a hex string to bytes, writes them to `out` and returns how many bytes it wrote. It accepts upper and lower case digits. If the string has an odd length or contains a character that is not a hex digit, return `-1`. The empty string decodes to 0 bytes.

The tests try many different byte strings, so a lookup of fixed examples will not work.
