`skip_spaces` is a loop on `**p`: `while (**p == ' ') { (*p)++; }`. Keep the parentheses.
===
`make_greeting` needs `strlen("Hello, ") + strlen(name) + 1 + 1` bytes: the prefix, the name, the `!` and the `'\0'`. Using `snprintf(buf, size, "Hello, %s!", name)` writes it all in one go. `take_word` can call `skip_spaces`, then find the word's length by walking a local pointer, then `malloc(len + 1)` and `memcpy`.
===
```c
int take_word(const char **p, char **out) {
    skip_spaces(p);
    const char *start = *p;
    while (**p != '\0' && **p != ' ') {
        (*p)++;
    }
    size_t len = (size_t)(*p - start);
    if (len == 0) {
        *out = NULL;
        return -1;
    }
    // allocate len + 1, copy len bytes, add '\0', store in *out
    return 0;
}
```
