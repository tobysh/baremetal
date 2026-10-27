For `digit_sum`, use a `while (n > 0)` loop. Each time round, add `n % 10` to a total and then replace `n` with `n / 10`.
===
For `count_divisible`, loop `x` from `lo` up to and including `hi` (that is `<=`, not `<`), and count the values where `x % d == 0`. If `lo > hi` the loop body never runs, so the answer is already 0.
===
For `is_prime`, return 0 straight away if `n < 2`. Then try `i` from 2 upwards: if `n % i == 0` the number is not prime. You can stop once `i * i > n`, and if no divisor was found, return 1.
