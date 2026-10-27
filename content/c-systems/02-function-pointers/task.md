Implement three functions that take a function pointer:

- `void map_ints(int *a, int n, int (*f)(int))` replaces every `a[i]` with `f(a[i])`.
- `int count_if(const int *a, int n, int (*pred)(int))` returns how many elements make `pred` return non-zero.
- `void sort_ints(int *a, int n, int (*cmp)(int, int))` sorts the array in place so that for neighbours, `cmp(a[i], a[i + 1]) <= 0`. A negative result from `cmp(x, y)` means `x` goes first. Do not use `qsort`.

`n` may be 0.
