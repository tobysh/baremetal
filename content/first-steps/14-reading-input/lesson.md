---
title: Reading input
summary: Read numbers typed by the user with scanf and respond to them.
minutes: 15
difficulty: medium
tags: input, strings
id: first-steps/reading-input
mode: stdout
---
So far our programs always do the same thing. Real programs react to **input**. The opposite of `printf` is `scanf`, which reads text and converts it into values:

```c
int age;
scanf("%d", &age);
printf("Next year you'll be %d\n", age + 1);
```

Two things to notice:

1. The format code `%d` means the same as in `printf`: "an int".
2. There's an `&` before `age`. `scanf` needs to know **where** `age` lives in memory so it can put the number there. `&age` means "the address of `age`". You'll learn exactly what that means in the *Pointers & Memory* track — for now, remember that `scanf` needs `&`.

## Reading several values

```c
int x, y;
scanf("%d %d", &x, &y);
```

`scanf` skips spaces and newlines between numbers, so this works whether the user types `3 4` or `3` and `4` on separate lines.

## Trying it out

The workspace has an **input** box under the editor. Whatever you type there is fed to your program when you press **Run**.
