Read stdin lines (at most 200 lines of at most 800 characters). For each line print `user=<value> src=<value> action=<value>` using the first occurrence of each whole key, with the quotes removed and `\"` and `\\` unescaped. Print `-` for a key that is absent (an empty value `src=` prints as an empty value, not `-`). Keys such as `username` must not match `user`, and text inside quoted values is never a key.

Tests use several different inputs, including decoys and missing keys.

Details the tests rely on: fields are separated by single spaces; a quoted value may be empty (`user=""` prints an empty value); a bare word with no `=` that is not one of the three keys (such as `stray`) is skipped; a line with none of the three keys prints `user=- src=- action=-`; an escape sequence other than `\"` and `\\` just keeps the character after the backslash. Output has exactly one line per input line.
