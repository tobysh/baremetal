---
title: Find the Bug
summary: Read six small functions that almost work, spot the off-by-one and other classic slips, and fix each one so the tests pass.
minutes: 20
difficulty: easy
tags: debugging, undefined-behaviour
id: c-practice/find-the-bug
mode: harness
---
Professional programmers spend far more time *reading* code than writing it, and most of that reading is hunting for a bug. Good news: the same few mistakes cause most of them. Learn their faces and you will spot them quickly.

## Off-by-one: the most famous bug

An array of `n` items has indices `0` to `n - 1`. Loops that start or stop one step too early or too late are called **off-by-one** errors. They hide well because the code *looks* right and works for most inputs.

```c
for (int i = 1; i < n; i++)    // skips item 0
for (int i = 0; i < n - 1; i++) // skips the last item
for (int i = 0; i <= n; i++)    // runs one past the end, which is undefined behaviour
```

When you suspect one, do the smallest test by hand: an array of **one** item. Which indices does the loop visit? Then try two.

## Bad starting values

Finding the biggest number? If you begin with `max = 0`, an array of only negative numbers will wrongly report `0`. Start from the first real element instead.

## Integer division

`7 / 2` is `3` in C, because dividing two ints throws the fraction away. If you want `3.5`, at least one side must be a `double`: `7 / 2.0`, or `(double)7 / 2`.

## Doing too much

Reversing an array by swapping each item with its mirror is a nice trick, but if you loop over the *whole* array you swap every pair twice and end up where you began.

## How to debug

1. Read the test message. It tells you the input and what you got.
2. Pick the smallest failing input and walk through the code by hand, writing down each variable.
3. Change one thing, then run again.

## Your turn

Each function below has exactly one bug. The code compiles and looks reasonable, so read it carefully. Fix all six.
