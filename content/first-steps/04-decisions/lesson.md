---
title: Making decisions with if
summary: Use if, else and comparison operators to make your program choose what to do.
minutes: 15
difficulty: easy
id: first-steps/decisions
mode: harness
---
Programs get interesting when they can **choose**. In C you do that with `if`:

```c
if (temperature > 30) {
    printf("Hot!\n");
} else if (temperature > 15) {
    printf("Nice.\n");
} else {
    printf("Cold.\n");
}
```

C checks each condition from top to bottom and runs the first block whose condition is true.

## Comparisons

| Operator | Meaning |
|---|---|
| `==` | equal to (two equals signs!) |
| `!=` | not equal to |
| `<` `>` | less than, greater than |
| `<=` `>=` | less/greater than or equal |

A classic beginner bug is writing `if (x = 5)` — that *assigns* 5 to `x` instead of comparing. Use `==` to compare.

## Combining conditions

- `&&` means **and**: `if (age >= 13 && age <= 19)`
- `||` means **or**: `if (day == 6 || day == 7)`
- `!` means **not**: `if (!finished)`

## True and false in C

C doesn't have a separate true/false type by default: `0` means false and any other number means true. A function that answers a yes/no question usually returns `1` for yes and `0` for no.

The `%` operator is handy here: `n % 2` is the remainder after dividing by 2, so it's `0` for even numbers.
