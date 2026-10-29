Read stdin lines (at most 200 lines of at most 800 characters). For each line print `user=<value> src=<value> action=<value>` using the first occurrence of each whole key, with the quotes removed and `\"` and `\\` unescaped. Print `-` for a key that is absent (an empty value `src=` prints as an empty value, not `-`). Keys such as `username` must not match `user`, and text inside quoted values is never a key.

Tests use several different inputs, including decoys and missing keys.
