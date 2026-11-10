Implement `void report(const char *name, const uint8_t *p, size_t n, uint64_t expected, char *out, size_t cap)`.

Write one line, ending in a newline, in exactly this format:

`NAME: TYPE, first bytes HEX, hash DIGEST, STATUS`

- `NAME` is `name` unchanged.
- `TYPE` is `PNG`, `JPEG`, `PDF` or `ZIP` when the evidence starts with the full signature (`89 50 4e 47 0d 0a 1a 0a`, `ff d8 ff`, `%PDF-`, `50 4b 03 04`) and `n` is at least its length; every other input, including GIF and ELF headers and truncated signatures, is `unknown`.
- `HEX` is the first `min(n, 8)` bytes, each as two lowercase hex digits, separated by single spaces with no trailing space. If `n == 0` it is `-`.
- `DIGEST` is the 64-bit FNV-1a checksum of all `n` bytes (start `0xcbf29ce484222325`, XOR the byte, multiply by `0x100000001b3`), printed as exactly sixteen lowercase hex digits.
- `STATUS` is `OK` when the computed digest equals `expected`, otherwise `CHANGED`.

Write at most `cap - 1` characters followed by a zero byte when `cap > 0` (a truncated report is a prefix of the full line); with `cap == 0` write nothing and `out` may be NULL. `name` and `p` are valid.

The hidden tests vary the type, the length (0, 1, 7, 8, 9 and longer), whether the digest matches, and the output capacity. Keep the supplied function name and types.
