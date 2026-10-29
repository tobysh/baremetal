**Challenge: memmove and strchr.**

Two more libc functions, and each has a trap.

- `void *my_memmove(void *dst, const void *src, size_t n)` copies `n` bytes and returns `dst`, **and the two regions may overlap**. Copying forwards is wrong when `dst` lies inside the source (you overwrite bytes you have not read yet), so in that case copy **backwards**, from the last byte to the first. `rep movsb` can do that with the direction flag set (`std`), but the System V ABI requires `DF` to be **clear** whenever you return or call another function, so use `cld` afterwards. `n` may be 0, and `dst == src` is fine.
- `char *my_strchr(const char *s, int c)` returns a pointer to the first byte of `s` equal to `(char)c`, or `NULL` (0) if there is none. Searching for `0` finds the terminator itself, so `my_strchr(s, 0)` returns `s + strlen(s)`. Compare as unsigned bytes: `c` is passed as an `int` and only its low 8 bits count (`c = 0x1E9` matches the byte `0xE9`). Never read past the terminator.

The hidden tests put the data flush against inaccessible pages, so reading or writing even one byte too far crashes. Preserve `rbx`, `rbp` and `r12`-`r15`.
