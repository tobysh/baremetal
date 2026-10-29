Loop with a pointer `p` starting at `path`. `const char *colon = strchr(p, ':')` gives the end of this entry (or `NULL` for the last one); the entry length is `colon ? colon - p : strlen(p)`. After handling it, set `p = colon + 1`, or stop if `colon` is `NULL`.
===
Build the candidate with `snprintf(out, out_size, "%.*s/%s", (int)len, p, name)`. If the return value is `>= out_size` it was truncated, so give up. Test with `stat` + `S_ISREG` + `access(out, X_OK)`.
===
`run_with_path` is lesson 2 again: `fork`, then in the child `find_in_path` into a local buffer and `execv(buf, argv)`; `_exit(127)` if either step fails.
