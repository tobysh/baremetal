Count the bytes. A buffer with `cap` bytes has valid indices `0` to `cap - 1`. How many of those are available for characters if one must hold the `'\0'`?
===
The loop should stop when `i` reaches `cap - 1`, so use `i < cap - 1`. Before the loop, return early when `dst == NULL` or `cap == 0`, otherwise `cap - 1` would wrap around to a huge number (`cap` is unsigned).
===
```c
if (dst == NULL || cap == 0) return;
if (src == NULL) src = "";
size_t i;
for (i = 0; i < cap - 1 && src[i] != '\0'; i++) {
    dst[i] = src[i];
}
dst[i] = '\0';
```
