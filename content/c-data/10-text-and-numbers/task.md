Don't use `atoi`, `strtol`, `sprintf` or `snprintf`.

- `int parse_int(char s[])` converts the text at the start of `s` to an int. An optional `+` or `-` may come first, then digits. It stops at the first character that is not a digit. If there are no digits at all, return 0. There is no leading whitespace.
- `void int_to_text(int n, char out[])` writes the decimal text of `n` into `out`, with a leading `-` for negatives and a terminator at the end. `out` is big enough (at least 12 characters). Zero is `"0"`.
