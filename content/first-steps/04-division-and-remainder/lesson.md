---
title: Division and remainder
summary: Learn that integer division drops the fraction, and use % to get what is left over.
minutes: 15
difficulty: easy
tags: variables, arithmetic
id: first-steps/division-and-remainder
mode: stdout
---
You met `+ - * /` in [Variables and types](/learn/first-steps/variables). Division has a surprise: when both numbers are integers, the answer is an integer too, and the fraction is **thrown away** (not rounded).

```c
printf("%d\n", 7 / 2);    // 3, not 3.5
printf("%d\n", 9 / 10);   // 0
```

Think of the long division you did at school: "7 divided by 2 is 3, remainder 1". C gives you both halves of that sum:

- `/` is the **quotient**: how many whole times it fits.
- `%` is the **remainder** (also called *modulo*): what is left over.

```c
printf("%d\n", 7 / 2);    // 3
printf("%d\n", 7 % 2);    // 1
```

## Why `%` is so useful

- **Even or odd:** `n % 2` is `0` for even numbers and `1` for odd.
- **Clock arithmetic:** `(hour + 5) % 12` wraps round after 12.
- **Splitting a quantity into units:** 275 minutes is `275 / 60` = 4 hours and `275 % 60` = 35 minutes left over.
- **Last digit of a number:** `n % 10`. (And `n / 10` drops the last digit.)

Never divide by zero: the program crashes (`/` and `%` by `0` are errors).

## A plan

When you convert a total into bigger units, take the big unit first with `/`, then use `%` to find what remains.
