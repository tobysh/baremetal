Write three functions. None of them may allocate memory.

- `size_t count_char(const char *s, char c)` returns how many times `c` appears in `s`. Counting `'\0'` gives 0.
- `void reverse_prefix(char *s, size_t n)` reverses the first `n` characters of `s` in place and leaves the rest alone. `n` is never larger than the length of `s`, and `n` of 0 or 1 changes nothing.
- `int is_phrase_palindrome(const char *s)` returns 1 if `s` reads the same both ways when you ignore case and skip every character that is not a letter or digit, otherwise 0. A string with no letters or digits counts as a palindrome.
