Loop with `while (scanf("%d", &x) == 1)`, and keep `count`, `sum` and `max` outside it.
===
Inside the loop, `count++` and `sum += x`.
===
For the maximum, if `count == 1` after the increment, this is the first number, so set `max = x`; otherwise only change `max` when `x > max`.
===
After the loop, print `max: none` if `count == 0`, else print `max`.
