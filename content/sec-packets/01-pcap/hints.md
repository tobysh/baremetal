Look at the first four bytes before decoding any integer. A small helper `rd(const uint8_t *p, int bytes, int big)` that you call with the byte order you detected keeps the rest of the code identical for both formats.
===
Check the global header in a fixed order: length at least 24, magic, version major and minor, snaplen nonzero, link type 1. Return -1 on the first failure.
===
Loop while `off < n`. At the top of each iteration, prove `n - off >= 16` before reading the record header, then advance `off` past the 16 header bytes before you test the captured length.
===
After the header, the three length rules are: `micro < 1000000`, `cap <= snaplen && cap <= orig`, and `cap <= n - off`. Then add `cap` to `off` and count the record. Reaching `off == n` exactly ends the loop with the count.
