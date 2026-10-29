Write `detect_v1` first, it is one line. Then notice it also flags a comment: that is fine for v1, it is what you are going to fix.
===
For v2 write a helper `scan(const char *text, size_t len, int depth)` that walks the lines. For each line: skip leading blanks, skip `#`, check for `PACK` (case-insensitively), otherwise normalise into a small buffer and compare.
===
Decoding: after `xor=` read two hex digits as the key, skip one space, then convert each hex pair to a byte and XOR it with the key. Put the result in a local buffer (at most 2000 bytes) and call `scan` on it with `depth + 1`. Refuse to decode at depth 3.
