Written permission is about who signed, not how nicely they asked. Statement 7 hinges on the word *control*. Go through the statements slowly, one at a time.
===
For the wildcard, take the entry without its leading `*`: that is `.lab.test`. A host matches when it is longer than that suffix and ends with it. Compare lengths before calling `memcmp` or `strcmp` on the tail.
===
```c
static int in_scope(const char *e, const char *t) {
    if (e[0] == '*' && e[1] == '.') {
        size_t el = strlen(e) - 1, tl = strlen(t);
        return tl > el && strcmp(t + tl - el, e + 1) == 0;
    }
    return strcmp(e, t) == 0;
}
```
Then check the four reasons in the stated order.
