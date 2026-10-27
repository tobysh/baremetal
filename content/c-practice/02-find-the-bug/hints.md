Try each function on an array of one item and then two items, tracing the loop by hand. The failing test names tell you which function to look at.
===
There are six distinct slips: a loop that starts at 1, a loop that stops one early, a starting value of `0` for a maximum, a comparison loop that stops one comparison too soon, a reverse that swaps every pair twice, and an integer division.
===
- `sum_all`: start `i` at `0`.
- `count_char`: loop until `s[i] != '\0'` (or `i < len`, not `len - 1`).
- `max_of`: `int best = a[0];` then loop from `1`.
- `is_sorted`: compare `a[i]` with `a[i + 1]` for every `i < n - 1`.
- `reverse`: loop `i < n / 2`.
- `average`: `(double)total / n`.
