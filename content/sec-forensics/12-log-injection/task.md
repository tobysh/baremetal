Implement `size_t log_escape(char *out, size_t cap, const char *in)`.

Walk `in` byte by byte (NULL means the empty string) and produce the escaped text:

- `\n` (0x0a) becomes the two characters `\n`; `\r` (0x0d) becomes `\r`.
- A backslash becomes two backslashes.
- Any other byte below 0x20, and 0x7f, becomes `\x` followed by two lowercase hex digits (a tab is `\x09`).
- Every other byte, including 0x80 and above, is copied unchanged.

Return the total length of the full escaped text, excluding the terminating zero, whether or not it fit.

Write only whole escapes. Stop writing at the first piece that does not fit while leaving room for the terminating zero, and write nothing further after that point, even if a later, shorter piece would fit. If `cap > 0`, always terminate the output; if `cap == 0` or `out` is NULL, write nothing (but still return the length).

The hidden tests sweep every capacity from 0 to just past the full length with guard bytes around the buffer, check the output is always a prefix of the full result, check that decoding the result gives back the input for pseudo-random strings, and try inputs such as forged log lines. Keep the supplied function name and types.
