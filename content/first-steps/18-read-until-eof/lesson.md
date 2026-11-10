---
title: Reading until the input ends
summary: Keep reading numbers with scanf until there are no more, and handle empty input.
minutes: 20
difficulty: medium
tags: input, loops, design
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

## Design it yourself

From this lesson on, you choose the shape of the program. The starter is empty: write `main` yourself, and add helper functions if they make it clearer. Follow the six steps of the design recipe:

1. **Understand the problem.** What goes in and what comes out?
2. **Write examples.** Work out the output for a normal list, a list of only negative numbers, and an empty one.
3. **Choose signatures.** One `main` is enough here. If you use helpers, decide their parameters first.
4. **Plan.** This is a **sentinel loop** (read until the input ends) around a **counter**, an **accumulator** (the sum) and a running biggest value.
5. **Code.** Write the plan as comments, then fill them in.
6. **Test.** Run it with your own inputs in the input box, including no input at all.
