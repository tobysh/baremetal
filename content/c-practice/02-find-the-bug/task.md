The starter has six functions with one bug each. Fix them so they behave as described:

- `int sum_all(const int *a, int n)` returns the sum of all `n` items (`0` for `n == 0`).
- `int count_char(const char *s, char c)` returns how many times `c` appears in `s`.
- `int max_of(const int *a, int n)` returns the largest item. Assume `n >= 1`. The items may all be negative.
- `int is_sorted(const int *a, int n)` returns `1` if the items are in non-decreasing order (equal neighbours are fine), otherwise `0`. Arrays of 0 or 1 items are sorted.
- `void reverse(int *a, int n)` reverses the array in place.
- `double average(const int *a, int n)` returns the exact average as a `double` (`2.5` for `{2, 3}`), or `0.0` if `n == 0`.

Do not read past the end of any array.
