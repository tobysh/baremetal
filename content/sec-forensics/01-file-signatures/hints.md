Build a small table of name, signature bytes and signature length, then loop over it. That keeps the seven formats from turning into seven copies of the same `if`.
===
Signatures contain zero and nonprintable bytes, so compare with `memcmp`, not `strcmp`. Both GIF variants need their own table row because they differ in one byte.
===
Test `n >= length` before calling `memcmp`. If the length is wrong the comparison reads past the evidence, and the tests place the missing byte in memory just beyond `n` to catch that.
===
Return a string literal from the table for a match and `"unknown"` after the loop. You never need an offset: only offset zero is examined.
