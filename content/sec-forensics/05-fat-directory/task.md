Keep `typedef struct {char name[13]; int deleted, is_dir; uint32_t size, cluster;} DirEntry;`. Implement `int read_dir(const uint8_t *p, size_t n, DirEntry *out, size_t max)`.

Process `p` as 32-byte records, in order, using only complete records (ignore a trailing fragment shorter than 32 bytes):

1. If byte 0 is `0x00`, stop: nothing at or after this record is read.
2. If attribute byte 11 equals `0x0f` or has bit `0x08` set, skip the record (long-name piece or volume label).
3. Otherwise build an entry:
   - `deleted` is 1 when byte 0 is `0xe5`, else 0. A deleted entry is kept, and its first name character is `_`.
   - `name`: bytes 0..7 with trailing spaces removed, then, only if bytes 8..10 still have text after trailing spaces are removed, a `.` and that extension. Names are ASCII; the string is zero terminated and fits in 13 bytes.
   - `is_dir` is 1 when bit `0x10` of byte 11 is set.
   - `cluster` is `(high16 << 16) | low16` with the high word at offset 20 and the low word at offset 26, little-endian.
   - `size` is the 32-bit little-endian value at offset 28.
4. Return the number of entries stored, at most `max`. Stop as soon as `max` entries are stored; with `max == 0` store nothing.

The hidden tests vary the directory contents, include terminators with stale records after them, label and long-name records, deleted directories, names with and without extensions, a trailing partial record and several `max` values. Keep the supplied type and function name.
