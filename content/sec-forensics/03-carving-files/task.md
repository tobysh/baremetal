Keep `typedef struct {const char *type; size_t offset, length;} Carved;`. Implement `size_t carve(const uint8_t *p, size_t n, Carved *out, size_t max)`.

Scan offsets from 0 upward. At each offset:

- If at least 3 bytes remain and they are `ff d8 ff`, look for the first `ff d9` that starts at or after the byte following the 3 header bytes. If found, the object is a `"JPEG"` ending just after that `d9`.
- Otherwise, if at least 8 bytes remain and they are the PNG signature `89 50 4e 47 0d 0a 1a 0a`, look for the first `IEND` starting at or after the byte following the signature. The object is a `"PNG"` ending 8 bytes after the start of `IEND` and only counts if all 8 of those bytes lie inside the evidence.
- When a complete object is found, store `{type, offset, length}` in `out` (type is one of the two string literals) and continue scanning at the end of that object. When no complete object starts at an offset, advance by one byte.

Return the number of objects stored. Never store more than `max` entries, never write to `out` when `max` is 0, never read at or beyond `p + n`. Empty evidence returns 0.

The hidden tests build images with junk of varying length, minimal objects, back-to-back objects, an object ending on the last byte, a truncated CRC and a header absorbed by an earlier object. Keep the supplied type and function name.
