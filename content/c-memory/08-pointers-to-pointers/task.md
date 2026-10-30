Write three functions. Different strings and names are tried, including edge cases such as empty strings.

- `void skip_spaces(const char **p)` advances `*p` past any run of space characters (`' '`), so that `**p` is the first non-space character, or `'\0'`.
- `int make_greeting(const char *name, char **out)` allocates a new string `"Hello, NAME!"` with `malloc`, stores it in `*out`, and returns 0. If allocation fails, set `*out` to `NULL` and return -1. The caller frees it.
- `int take_word(const char **p, char **out)` skips spaces, then copies the next word (the characters up to the next space or the end of the string) into a new heap string stored in `*out`, and moves `*p` to just after that word. Return 0 on success. If there are no more words, set `*out` to `NULL`, leave `*p` at the end of the string, and return -1.
