Don't use `<string.h>`.

- `int str_equal(char a[], char b[])` returns `1` if the two strings hold exactly the same text, otherwise `0`.
- `int str_compare(char a[], char b[])` returns a negative number if `a` comes before `b` in dictionary (character code) order, `0` if they are equal, or a positive number if `a` comes after `b`. A string that is a prefix of another comes first. Compare characters as `unsigned char`.
- `void str_copy(char dst[], char src[])` copies `src`, terminator included, into `dst`.
- `void str_append(char dst[], char src[])` adds the text of `src` to the end of the text already in `dst`. `dst` always has room.
