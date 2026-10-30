`sum_between` is the loop from the lesson: start `p` at `begin`, stop when `p == end`, and add `*p` each time.
===
`find_max` can remember a pointer `best` instead of a value. Start with `best = arr`, walk `p` from `arr + 1` up to `arr + n`, and whenever `*p > *best` set `best = p`. Return `NULL` first if `n == 0`. `gap` is a single subtraction.
===
```c
const int *best = arr;
for (const int *p = arr + 1; p < arr + n; p++) {
    if (*p > *best) {
        best = p;
    }
}
return best;
```
