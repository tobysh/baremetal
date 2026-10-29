Loop `i` from 1 to `n - 1`. Save `a[i]` as `key`, then walk `j` leftwards while `a[j] > key`.
===
Inside the inner loop, copy `a[j]` to `a[j + 1]` and add one to a `shifts` counter. After the loop, store `key` at `a[j + 1]`. Return `shifts`.
===
The string version is the same code with `const char *key = a[i];` and the condition `j >= 0 && strcmp(a[j], key) > 0`.
