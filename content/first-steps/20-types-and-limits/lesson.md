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

Go past the top and the number does not stop, it **overflows**. For a signed `int` the C standard says nothing about what happens. That is called **undefined behaviour**: the language makes no promise, so the program may print a wrong number, print a different wrong number when compiled another way, or do something stranger, because the compiler is allowed to assume it never happens.

```c
int big = 2000000000;
int sum = big + big;     // not 4000000000
```

Compiled normally on a typical computer, this quietly gives `-294967296` with no warning. On this site your code is built with a checker turned on, so the same lines stop the program with a message instead:

```text
main.c:5:9: runtime error: signed integer overflow: 2000000000 * 2 cannot be represented in type 'int'
```

Read it like any error: the file and line, then what went wrong. The checker only helps you find the mistake. The cure is to choose a type big enough in advance.

## What's in memory

An `int` is a 4-byte box, which can hold only 2^32 different bit patterns. The sum 4000000000 needs a bigger box. A `long long` box has 8 bytes:

```text
int       [ 4 bytes ]            range about -2.1 billion .. +2.1 billion
long long [ 8 bytes            ]  range about -9.2 quintillion .. +9.2 quintillion
```

Undefined behaviour is not only about overflow. Reading a variable you never set, indexing past the end of an array, and using a pointer that points nowhere are all the same kind of mistake. Later lessons show each one.

## Bigger and unsigned types

`long long` is 8 bytes and holds values up to about 9.2 × 10^18. Its `printf` and `scanf` code is `%lld`:

```c
long long a;
scanf("%lld", &a);
printf("%lld\n", a * 2);
```

`unsigned int` has no negative values, so it holds 0 to about 4.3 billion in the same 4 bytes. Mixing signed and unsigned values in a comparison is a source of bugs, so stay with signed types until you have a reason.

Do the arithmetic in the big type: converting an `int` result to `long long` after the overflow has already happened is too late. Compare against the limit as a `long long` as well, for example `sum >= INT_MIN && sum <= INT_MAX`.
