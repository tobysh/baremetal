---
title: Decimal numbers
summary: Use double for numbers with a fractional part, print them with %f, and avoid the integer-division trap.
minutes: 15
difficulty: easy
tags: variables, types
id: first-steps/decimals
mode: stdout
---
`int` holds whole numbers only. For anything with a fractional part (a price, a temperature, an average) use **`double`**:

```c
double price = 4.50;
double total = price * 3;      // 13.5
printf("%f\n", total);         // 13.500000
```

The format code for a `double` is `%f`. By default it prints six digits after the point. Put `.2` between the `%` and the `f` to choose **two**:

```c
printf("%.2f\n", total);       // 13.50
```

## The integer-division trap

If both sides of a `/` are integers, C does *integer* division, even if you store the answer in a `double`:

```c
double wrong = 7 / 2;          // 3.0 (the division happened first, with ints)
double right = 7 / 2.0;        // 3.5 (one side is a double, so the whole sum is)
```

A decimal point in the number (`2.0`) makes it a `double`. You can also convert a variable with a **cast**: `(double)count / 3`.

## Computers store decimals approximately

`0.1` cannot be stored exactly in binary, just as 1/3 cannot be written exactly in decimal. That is why we print with `%.2f` rather than trusting every digit, and why you should never expect `0.1 + 0.2 == 0.3` to be true.
