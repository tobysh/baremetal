Keep the supplied `Segment` struct. Implement `int http_path(const Segment *s, size_t count, uint32_t base, char *out, size_t cap)`. The segments are one direction of one connection, in any order, with sequence numbers that do not wrap. `base` is the sequence number of the first stream byte.

**Return value:** the length of the path (a positive number) on success, with `out` holding the path and a terminating NUL. On any failure return -1 and leave `out` unchanged.

**Reassembly (at most 1024 bytes):**

- A segment with `n == 0` is ignored. A segment with a NULL `data` pointer and `n > 0` is an error.
- A segment with `seq < base` is an error. Its offset is `seq - base`, and offset plus `n` must not exceed 1024.
- Bytes written twice are fine if they are identical. A different value at the same offset is an error.
- Let the stream end be the greatest offset plus length. Every byte from 0 up to that end must have been supplied (no gaps).
- Only printable ASCII (32 to 126) and CR/LF are accepted in the stream. Any other byte is an error.

**HTTP subset:** the stream must begin with `GET `, then a non-empty path containing no bytes below 33, then ` HTTP/1.1\r\n`, and must contain a blank line (`\r\n\r\n`). Bytes after that (a body) are ignored. `POST`, other versions and lowercase `get` are errors.

**Output:** `out` must be non-NULL and `cap` must be at least path length plus 1. Otherwise return -1. A NULL `s` with `count == 0` is an empty stream and returns -1.

The tests shuffle, split and repeat segments, change the path, and test the 1024-byte limit, gaps, conflicts, and each output-size boundary. Keep the supplied function name and types.
