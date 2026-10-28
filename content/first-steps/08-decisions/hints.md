For `max_of_three`, start by assuming `a` is the biggest. Then check whether `b` is bigger, and then whether `c` is bigger.
===
```c
int best = a;
if (b > best) best = b;
```
Now do the same for `c`.
===
For `is_even`: a number is even when `n % 2 == 0`. In C, `-3 % 2` is `-1`, not `1` — so compare against 0 rather than checking for 1.
