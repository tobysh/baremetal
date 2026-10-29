`index_of` loops upward and returns `i` the moment `a[i] == target`; after the loop, return `-1`. `last_index_of` is the same loop run backwards: start `i` at `n - 1` and count down while `i >= 0`.
===
For `is_sorted`, compare each element with the next one. Loop while `i + 1 < n` so that `a[i + 1]` stays inside the array, and return 0 as soon as `a[i] > a[i + 1]`. Return 1 only after the loop. That also makes length 0 and 1 come out as sorted.
===
```c
int is_sorted(int a[], int n) {
    for (int i = 0; i + 1 < n; i++) {
        if (a[i] > a[i + 1]) return 0;
    }
    return 1;
}
```
