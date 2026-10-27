Both functions use the accumulator pattern from the lesson: a variable that starts at a sensible value and gets updated each time round the loop.
===
For `sum_to`, start `total` at 0. For `factorial`, start `result` at 1 — starting at 0 would make every product 0!
===
```c
long result = 1;
for (int i = 2; i <= n; i++) {
    result *= i;
}
return result;
```
