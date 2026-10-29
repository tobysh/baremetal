Walk `in` with an index. If `in[i] != '$'`, append it. Otherwise look at `in[i+1]`: `?` means format `last_status` with `snprintf` into a small buffer and append that; `{` means find the closing `}`; a letter or `_` means scan the name.
===
Copy the name into a local `char name[256]` (check the length) so you can call `getenv(name)`. Append the value character by character, or with a helper `append(const char *s, size_t n)` that returns failure if it would overflow.
===
For `do_export`: find the `=` with `strchr`, validate the characters before it, copy that part into a temporary buffer, and call `setenv(name, eq + 1, 1)`. Share the name validator between `do_export` and `do_unset`.
