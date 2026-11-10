Keep the supplied `Question` struct. Implement `int dns_question(const uint8_t *p, size_t n, Question *out)` that decodes the first (and only) question of a DNS message.

**Return value:** 0 on success, -1 on failure. On failure `*out` is unchanged. NULL `p` or `out` returns -1.

**Header (offset 0):** require `n >= 12` and QDCOUNT (bytes 4..5, big-endian) equal to exactly 1. The transaction ID is bytes 0..1; flags, answer, authority and additional counts are ignored.

**Name (starting at offset 12):** a sequence of labels, each a one-byte length followed by that many bytes, ending with a zero length byte.

- A length of 64 or more (this includes compression pointers, which start with the bits `11`) is rejected.
- A length larger than the bytes remaining is rejected. A name that runs off the end before its zero byte is rejected.
- Label bytes must be ASCII letters, digits or `-`. Anything else is rejected. Case is preserved.
- Join the labels with `.` into `name`. The display name may be at most 253 characters; longer is rejected. A name with no labels (just the zero byte) is stored as `.`.

**After the name:** require four more bytes. `type` is the next two bytes and `qclass` the two after that, both big-endian. Bytes after the question (answers and so on) are ignored.

`name` is a NUL-terminated string in a 254-byte array. The tests include labels of exactly 63 bytes, names of exactly 253 and 254 characters, truncation at every kind of boundary, and varied names. Keep the supplied function name and types.
