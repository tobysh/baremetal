Implement these functions. They are tested with ASCII and with multi-byte text such as `"héllo"` and `"世界"`.

- `RuneCount(s string) int` returns the number of characters (runes) in `s`, not the number of bytes.
- `Reverse(s string) string` returns `s` with its characters in reverse order, without breaking multi-byte characters.
- `Truncate(s string, n int) string` returns the first `n` characters of `s`. If `s` has `n` or fewer characters, or `n` is negative, treat it sensibly: return `s` unchanged for short strings and `""` when `n <= 0`.
- `IsPalindrome(s string) bool` reports whether `s` reads the same forwards and backwards when you look only at **letters** and ignore case. Digits, spaces and punctuation are ignored. An input with no letters counts as a palindrome.
- `Shift(s string, k int) string` is a Caesar cipher: every ASCII letter moves `k` places along the alphabet, wrapping around and keeping its case. `k` may be negative or larger than 26. Every other character, including non-ASCII ones, is left unchanged.
