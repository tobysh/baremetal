Two jobs: a glob matcher for one pattern and one string, and a loop that walks the allow lines in order for each alert, stopping at the first entry where all three globs match.
===
Write `glob_match(pattern, text)` recursively. Empty pattern means the text must be empty. For `*`, try the rest of the pattern against every suffix of the text, including the empty one. For `?` or an equal character, move both pointers forward by one.
===
If `ab?` matches `ab`, your `?` accepts the end of the string. If a `*` at the end of a pattern fails, you do not try the empty suffix. If hit counts are too high, you count every matching entry instead of only the first.
===
The recursion on a different example, a matcher that supports only `*`:
```c
static int match(const char *p, const char *s) {
    if (!*p) return !*s;
    if (*p == '*') { for (;; s++) { if (match(p + 1, s)) return 1; if (!*s) return 0; } }
    return *s == *p && match(p + 1, s + 1);
}
```
