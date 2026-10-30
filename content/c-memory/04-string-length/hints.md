Start counting at 0 and keep going while the current character is not `'\0'`.
===
You need a counter, `i`. Each time round the loop, if `s[i]` is not `'\0'`, you have found one more character, so add one to `i`. When the loop stops, `i` is the length.
===
```c
size_t n = 0;
while (s[n] != '\0') {
    ...
}
return n;
```
