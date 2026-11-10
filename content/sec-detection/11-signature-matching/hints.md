Two separate jobs: decode a hex string into bytes (strictly), and search for a byte pattern inside a window of those bytes. Write them as two small functions.
===
The window is `[offset, end)` where `end` is the payload length, or `offset + depth` if that is smaller and the depth is not zero. Try every start position `st` from `offset` while `st + pattern_length <= end`, and compare the pattern byte by byte.
===
If a pattern that only partly fits the window matches, your loop condition checks the start but not the end of the match. If an offset past the payload crashes or wraps around, check `size_t` arithmetic before subtracting. If uppercase patterns never match with `nocase`, fold both sides, not just the payload.
===
Decoding one pair of hex digits, on a different example:
```c
static int hv(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;                      /* invalid digit */
}
int byte = hv(s[0]) * 16 + hv(s[1]);  /* check both for -1 first */
```
