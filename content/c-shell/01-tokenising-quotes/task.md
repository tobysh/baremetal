Implement `int tokenize(const char *line, char *argv[], int max_args, char *buf, size_t buf_size)`.

It splits `line` into words using the shell rules above and stores each word, as a C string, in `buf`. `argv[i]` points at word `i` inside `buf`, and `argv[argc]` is set to `NULL`. `argv` has room for `max_args + 1` pointers. Do not modify `line`, and do not use `strtok`.

- Spaces and tabs separate words when not quoted. Leading, trailing and repeated separators are ignored.
- `'...'` keeps everything literally. `"..."` keeps everything literally except `\"` (a quote) and `\\` (a backslash); any other backslash inside double quotes stays as a backslash.
- Outside quotes, `\x` yields the character `x`. A lone backslash at the very end of the line is kept as a backslash.
- Adjacent quoted and unquoted pieces form one word. `""` is an empty word.
- Return the number of words on success. Return `-1` if a quote is not closed. Return `-2` if there are more than `max_args` words or the words do not fit in `buf` (including their terminators). The contents of `argv` and `buf` are unspecified after an error.
