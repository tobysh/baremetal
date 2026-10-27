Both functions follow the same recipe: work out how many bytes you need, call `malloc`, return `NULL` if it gave you `NULL`, fill the memory in, and return the pointer.
===
`make_range` needs `n * sizeof(int)` bytes. `duplicate` needs `strlen(s) + 1` bytes, because the `'\0'` takes a byte too. For the copy you can use a loop, or `memcpy`, or `strcpy` (only because you reserved the extra byte).
===
```c
size_t len = strlen(s);
char *copy = malloc(len + 1);
if (copy == NULL) {
    return NULL;
}
// copy len + 1 bytes from s to copy (so the '\0' comes along)
return copy;
```
