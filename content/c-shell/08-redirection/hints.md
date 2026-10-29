Check for syntax errors first, in the parent, by scanning `argv` once. Then `fork`; everything else (opening files, `dup2`, building the clean argv) can happen in the child.
===
Build `char *clean[N]` in the child as you scan: copy a word into `clean` unless it is an operator, in which case consume the following word as the filename and `open` it. Use `dup2(fd, 0)` for `<` and `>`/`>>`, `dup2(fd, 2)` for `2>`, then `close(fd)`.
===
The array can be as long as `argv`; count the words first and use a variable-length array `char *clean[count + 1]`. End with `clean[k] = NULL`; if `k == 0`, `_exit(0)`.
