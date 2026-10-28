---
title: And, or, not
summary: Combine conditions with &&, || and !, and use them to decide whether a year is a leap year.
minutes: 15
difficulty: easy
tags: conditionals, logic
id: first-steps/logical-operators
mode: stdout
---
In [Making decisions with if](/learn/first-steps/decisions) each `if` tested one thing. Real questions combine several: "is it a weekday **and** before 9?", "is the letter `a` **or** `e`?". C has three operators for that:

| Operator | Means | True when |
|---|---|---|
| `a && b` | and | both are true |
| `a \|\| b` | or | at least one is true |
| `!a` | not | `a` is false |

```c
if (age >= 13 && age <= 19) {
    printf("teenager\n");
}
if (letter == 'a' || letter == 'e') {
    printf("a or e\n");
}
if (!(score > 50)) {
    printf("50 or below\n");
}
```

Note that you must repeat the variable: `age >= 13 && <= 19` is not C. Each side of `&&` and `||` is a complete test.

## Short-circuiting and brackets

`&&` and `||` stop as soon as the answer is known. And just like in maths, brackets say what groups with what; `&&` binds tighter than `||`, so when you mix them, add brackets to make your intent obvious.

## A real rule: leap years

A year is a leap year if it is divisible by 4, **except** century years, which must also be divisible by 400. So 2024 and 2000 are leap years, but 1900 and 2023 are not.

In C: `(year % 4 == 0 && year % 100 != 0) || year % 400 == 0`. A year is divisible by `n` when `year % n == 0` (see [Division and remainder](/learn/first-steps/division-and-remainder)).
