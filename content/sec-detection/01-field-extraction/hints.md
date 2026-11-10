Think of the line as a stream of `key=value` units. Your loop should consume one whole unit at a time, so that you always know whether you are looking at a key or at the inside of a value.
===
Write a helper that, given a position, skips spaces, reads the key up to `=` or a space, then reads either a quoted or a bare value. Call it in a loop until the end of the line. Keep a `have[]` flag per wanted key so only the first occurrence is stored.
===
If `username=deploy` shows up as the user, you compared only the first four bytes of the key; compare the key length as well. If a quoted `user=root` leaks out, you are scanning inside the quotes for keys instead of consuming the quoted value in one step.
===
A quoted-value reader looks like this (on a different task, reading a `tag`):
```c
if (*p == '"') {
    p++;
    while (*p && *p != '"') {
        if (*p == '\\' && p[1]) p++;   /* keep the escaped character */
        val[n++] = *p++;
    }
    if (*p == '"') p++;
}
```
