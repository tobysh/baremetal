Write two functions. Neither may modify the string.

- `int is_palindrome(const char *s)` returns 1 if `s` reads the same forwards and backwards using exact character comparison (case-sensitive; spaces and punctuation count as characters), otherwise 0. `""` and any single character are palindromes.
- `int is_palindrome_ignoring_case(const char *s)` does the same but treats upper and lower case letters as equal. Use `tolower` from `<ctype.h>` and cast each character to `unsigned char` first.

`s` is never NULL.
