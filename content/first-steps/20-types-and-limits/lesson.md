---
title: Types and their limits
summary: See how big int is, what happens when a number doesn't fit, and when to use long long.
minutes: 20
difficulty: medium
tags: types, integers
id: first-steps/types-and-limits
mode: stdout
---
A variable lives in a fixed number of bytes. That fixes the range of values it can hold.

## How big is it?

`sizeof` gives the size of a type in bytes:

```c
printf("%d\n", (int)sizeof(int));        // 4 on most computers
printf("%d\n", (int)sizeof(long long));  // 8
```

A 4-byte `int` holds numbers from about -2.1 billion to +2.1 billion. The exact limits are in `<limits.h>` as `INT_MIN` and `INT_MAX` (2147483647).

## Overflow

Go past the top and the number does not stop, it **overflows**. For a signed `int` the C standard doesn't say what happens, and in practice the result is garbage, often a large negative number:

```c
int big = 2000000000;
int sum = big + big;     // not 4000000000
```

The compiler doesn't warn you, so you have to choose a type big enough in advance.

## Bigger and unsigned types

`long long` is 8 bytes and holds values up to about 9.2 × 10^18. Its `printf` and `scanf` code is `%lld`:

```c
long long a;
scanf("%lld", &a);
printf("%lld\n", a * 2);
```

`unsigned int` has no negative values, so it holds 0 to about 4.3 billion in the same 4 bytes. Mixing signed and unsigned values in a comparison is a source of bugs, so stay with signed types until you have a reason.

Do the arithmetic in the big type: converting an `int` result to `long long` after the overflow has already happened is too late. Compare against the limit as a `long long` as well, for example `sum >= INT_MIN && sum <= INT_MAX`.
