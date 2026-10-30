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
