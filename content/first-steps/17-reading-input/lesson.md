---
title: Reading input
summary: Read numbers typed by the user with scanf and respond to them.
minutes: 15
difficulty: medium
tags: input, strings
id: first-steps/reading-input
mode: stdout
---
In [Reading a number](/learn/first-steps/reading-a-number), you used `scanf("%d", &number)` to read one integer. Now let's read two values in one call and combine input, arithmetic and decisions.

## Reading several values

```c
int x, y;
if (scanf("%d %d", &x, &y) != 2) {
    return 1;
}
```

There are two `%d` format codes and two variable addresses, in the same order: the first number goes into `x`, the second into `y`. We check for a return value of `2` because both reads must succeed before we use the variables.

`scanf` skips spaces and newlines between numbers, so this works whether the user types `3 4` or `3` and `4` on separate lines.

## Trying it out

The workspace has an **input** box under the editor. Whatever you type there is fed to your program when you press **Run**.

## A worked example

Here is a complete program that reads two numbers and tells you the bigger one. Read it once, then trace it with the input `8 3`.

```c
#include <stdio.h>

int main(void) {
    int x, y;
    if (scanf("%d %d", &x, &y) != 2) {
        return 1;
    }
    if (x > y) {
        printf("bigger: %d\n", x);
    } else if (y > x) {
        printf("bigger: %d\n", y);
    } else {
        printf("bigger: equal\n");
    }
    return 0;
}
```

Step by step, for the input `8 3`:

1. `int x, y;` makes two empty boxes. They hold garbage until something fills them.
2. `scanf` reads `8` into `x` and `3` into `y`, and returns `2` because it filled two boxes. `2 != 2` is false, so we carry on.
3. `x > y` is `8 > 3`, which is true, so it prints `bigger: 8`. The `else if` and `else` are skipped.

Now try the input `5` with nothing after it. `scanf` fills `x` with 5, finds no second number, and returns `1`. The check `1 != 2` is true, so the program stops at `return 1;` and never uses the empty `y`. That check is what keeps a half-typed input from producing nonsense.

The three-way decision is worth remembering: with two numbers there are three outcomes (first bigger, second bigger, equal), and an `if`, `else if`, `else` chain covers each exactly once. Your task is this program with an added `sum:` line, so when you get stuck, trace the numbers through it by hand.

Two habits to keep: always check the number `scanf` returns, and always put `&` before the variable it fills.
