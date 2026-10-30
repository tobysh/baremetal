Declare `long long a, b;` and read them with `scanf("%lld %lld", &a, &b)`.
===
Print the sizes with `printf("int: %d bytes\n", (int)sizeof(int));` and the same for `long long`.
===
Print the sum with `%lld`. Compute `long long sum = a + b;` first.
===
The last line: `sum >= INT_MIN && sum <= INT_MAX` is true when it fits. Both limits are `int` values, and comparing them with a `long long` works correctly.
