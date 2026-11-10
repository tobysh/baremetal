Implement `size_t hexdump(const uint8_t *p, size_t n, char *out, size_t cap)`.

For each group of up to 16 bytes at offset `off`, produce one row:

1. `%08zx: ` (eight lowercase hex digits for the offset, a colon, a space).
2. The bytes as lowercase two-digit hex, one space between each pair of bytes (after every second byte, none at the end), then spaces so this field is exactly 39 characters.
3. Two spaces.
4. One character per byte: the byte itself when it is 0x20 to 0x7e inclusive, otherwise `.`.
5. A newline.

Example, one byte `41`: `00000000: 41` followed by 37 spaces, two spaces, `A` and a newline (the 39-character field is `41` plus 37 spaces).

Empty evidence (`n == 0`) produces the empty string. Rows are not separated by blank lines and there is no trailing row after the last byte.

Output contract, like `snprintf`: return the full length the complete dump needs, excluding the terminating zero, whether or not it fit. Write at most `cap - 1` characters followed by a zero byte when `cap > 0`; the written text is the prefix of the full dump. When `cap == 0`, write nothing and `out` may be NULL. `p` is valid for `n` bytes.

The hidden tests include a capacity sweep with guard bytes around the buffer, all 256 byte values, and an input long enough for a five-digit offset. Keep the supplied function name and types.
