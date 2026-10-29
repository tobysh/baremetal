Implement three functions.

- `int expand_word(const char *in, char *out, size_t out_size, int last_status)` copies `in` to `out`, replacing `$NAME`, `${NAME}` and `$?` as in the table (unset variables become empty, `$?` becomes `last_status` in decimal). A `$` not followed by a letter, `_`, `{` or `?` is copied as is. An unterminated `${NAME` is copied literally. Return the length of the result, or `-1` if it does not fit in `out_size` bytes including the terminator.
- `int do_export(const char *assignment)` takes `NAME=value`, validates the name (letter or `_`, then letters, digits or `_`), and sets it with `setenv`. The value may be empty. Return `0` on success, `1` if the name is invalid or there is no `=`. Do not modify `assignment`.
- `int do_unset(const char *name)` removes the variable. Return `0`, or `1` if the name is invalid.
