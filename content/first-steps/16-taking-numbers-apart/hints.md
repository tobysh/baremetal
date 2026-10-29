Read with `scanf("%d", &n);`, then loop `while (n > 0)`.
===
Inside the loop: `int digit = n % 10; count++; sum += digit; reversed = reversed * 10 + digit; n /= 10;`.
===
Declare `count`, `sum` and `reversed` as `0` before the loop and print them after it.
