Read two integers. They can be as large as a few billion, so an `int` isn't enough. Print three lines:

```
int: 4 bytes
long long: 8 bytes
sum: 4000000000
fits in int: no
```

The first two lines are the sizes from `sizeof`. `sum` is the sum of the two numbers, and the last line says whether that sum fits in an `int` (`yes` or `no`, using `INT_MIN` and `INT_MAX` from `<limits.h>`).

Other inputs are tried, including big negative numbers and sums sitting right at the limit.
