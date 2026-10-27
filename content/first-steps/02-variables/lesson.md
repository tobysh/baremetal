---
title: Variables and types
summary: Store numbers in variables, do arithmetic, and print the results with printf format codes.
minutes: 15
difficulty: easy
id: first-steps/variables
mode: stdout
---
A **variable** is a named box that holds a value. In C, every box has a **type** that says what kind of value fits in it and how much memory it takes.

```c
int apples = 3;      // a whole number (integer)
double price = 0.5;  // a number with a decimal point
char grade = 'A';    // a single character
```

`int apples = 3;` reads as: "make a box called `apples` that holds an `int`, and put 3 in it." You can change what's in the box later:

```c
apples = apples + 2;  // now 5
```

The `=` here means **assign** (put into the box), not "is equal to".

## Arithmetic

C has the usual operators: `+`, `-`, `*`, `/`, and `%` (remainder). One surprise: dividing two `int`s throws away the fraction. `7 / 2` is `3`, not `3.5`. And `7 % 2` is `1` — the remainder.

## Printing values

`printf` uses **format codes** as placeholders. `%d` means "an int goes here", `%f` means "a double goes here".

```c
int a = 4;
int b = 6;
printf("%d times %d is %d\n", a, b, a * b);
// prints: 4 times 6 is 24
```

The values after the text fill the placeholders in order.
