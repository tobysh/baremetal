`copy_label(dst, cap, src)` is meant to copy the text in `src` into a buffer of `cap` bytes, truncating it if it does not fit. The version in the editor has an off-by-one bug. Fix it so that:

- at most `cap - 1` characters are copied, followed by one `'\0'`, so the result is always a valid string that fits in `cap` bytes;
- nothing at all is written at or beyond `dst[cap]`;
- if `cap` is `0`, or `dst` is `NULL`, the function does nothing;
- if `src` is `NULL`, it stores an empty string.

Other buffer sizes and many different strings are tried, including ones much longer than the buffer.
