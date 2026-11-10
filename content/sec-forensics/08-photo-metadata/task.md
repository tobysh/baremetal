Keep `typedef struct {char make[64], model[64]; uint32_t width, height; char datetime[32];} Meta;`.

Implement `int read_meta(const uint8_t *p, size_t n, Meta *out)` for the TIFF subset in the lesson.

- Return -1 if `n < 8`, the first two bytes are not `II` or `MM`, the 16-bit number at offset 2 is not 42, the directory offset leaves fewer than 2 bytes in the data, or the entries do not fit: `count * 12` bytes must lie within the data after the 2-byte count.
- Entries are tag, type, count, value as in the lesson. Handle these tags and ignore everything else (unknown tags, and supported tags with an unsupported type):
  - Make `0x010f`, Model `0x0110`, timestamp `0x0132`, type 2 (ASCII): `count` includes the terminating zero. With `count <= 4` the bytes are in the value field, otherwise the value is an offset into the data. Return -1 if `count` is 0, the string does not fit its field (`make`/`model` 64 bytes, `datetime` 32, terminator included), its bytes are outside the data, or its last byte is not zero or it contains an earlier zero.
  - Width `0x0100` and height `0x0101`, type 3 (SHORT) or 4 (LONG): `count` must be 1, otherwise return -1. A SHORT is the first two bytes of the value field in the file's byte order.
- On success (return 0) every field that was not present is empty or zero, whatever `*out` held before. On failure `*out` may hold anything.

Implement `long long to_unix(const char *s)`: `s` must be exactly `YYYY:MM:DD HH:MM:SS` (colons, one space, digits only), a real Gregorian date with year 1970 to 2100, hour 0 to 23, minute and second 0 to 59. Return the seconds since 1970-01-01 00:00:00 UTC, or -1 for anything else.

The hidden tests build the same photo in both byte orders, vary string lengths around the 4-byte inline limit, corrupt offsets and counts, and compare `to_unix` with a reference for every day from 1970 to 2100. Keep the supplied type and function names.
