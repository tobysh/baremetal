Write one function, without calling `strlen`:

- `size_t my_strlen(const char *s)` returns the number of characters in `s` before the terminating `'\0'`. The terminator is not counted. The empty string `""` has length 0.

`s` is never NULL.

Your function is also tried on fresh random strings, including strings with bytes after the first terminator and pointers that start inside a character array.
