Plan the memory first. `repeat_str` needs `strlen(s) * times + 1` bytes. `evens` can never need more than `n` ints, so allocate `n` (or 1 if `n` is 0) and fill only as many as you find.
===
For `repeat_str`, keep a write position and copy `s` into it `times` times with `memcpy`, then add the `'\0'`. For `evens`, count as you copy and set `*out_count` at the end. `min_max` starts both outputs at `arr[0]` and compares the rest.
===
```c
size_t len = strlen(s);
char *out = malloc(len * (size_t)times + 1);
if (out == NULL) {
    return NULL;
}
char *w = out;
for (int i = 0; i < times; i++) {
    memcpy(w, s, len);
    w += len;
}
*w = '\0';
return out;
```
