Write two functions that return memory allocated with `malloc`. The caller will `free` it.

- `int *make_range(int n)` returns a heap array of `n` ints holding `0, 1, 2, ..., n - 1`. `n` is at least 1. Return `NULL` if allocation fails.
- `char *duplicate(const char *s)` returns a newly allocated copy of the string `s`, including its terminating `'\0'`. Allocate `strlen(s) + 1` bytes. Return `NULL` if allocation fails. Do not use `strdup`.

The returned copy must be independent: changing the original afterwards must not change the copy.
