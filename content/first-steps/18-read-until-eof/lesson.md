---
title: Reading until the input ends
summary: Keep reading numbers with scanf until there are no more, and handle empty input.
minutes: 20
difficulty: medium
tags: input, loops
id: first-steps/read-until-eof
mode: stdout
---
So far you knew how many numbers to read. Often you don't: the input just goes on until it runs out. That end is called **EOF** (end of file).

## scanf tells you when it fails

`scanf` returns how many values it read. With one `%d` it returns `1` on success, and something else when the input has ended (or holds something that isn't a number). That makes a natural loop:

```c
int x;
while (scanf("%d", &x) == 1) {
    printf("got %d\n", x);
}
```

The loop runs once per number, however many there are, even zero.

## Keeping a summary

Combine the loop with a few variables that update on every pass:

```c
int count = 0, sum = 0, x;
while (scanf("%d", &x) == 1) {
    count++;
    sum += x;
}
```

## The first value is special

A running maximum needs a starting value. If you start it at `0`, a list of only negative numbers gives the wrong answer. Use the first number you read, tracked with a flag or by checking `count == 0`, as the starting maximum.

Also decide what to print when there was no input at all. Check `count` after the loop rather than assuming at least one number arrived.
