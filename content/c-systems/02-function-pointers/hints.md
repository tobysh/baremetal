`map_ints` is a loop with `a[i] = f(a[i]);`. `count_if` is a loop with `if (pred(a[i])) count++;`. Call a function pointer exactly like a normal function.
===
For `sort_ints`, insertion sort: for each `i` from 1, take `key = a[i]`, then shift earlier elements right while `cmp(a[j], key) > 0`, and drop `key` into the gap.
===
```c
for (int i = 1; i < n; i++) {
    int key = a[i];
    int j = i - 1;
    while (j >= 0 && cmp(a[j], key) > 0) {
        a[j + 1] = a[j];
        j--;
    }
    a[j + 1] = key;
}
```
