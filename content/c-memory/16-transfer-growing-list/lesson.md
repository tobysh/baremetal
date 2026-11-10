---
title: "Transfer: Numbers of Unknown Size"
summary: Solve a small problem from a written description alone, choosing your own functions and managing heap memory that grows as the input does.
minutes: 35
difficulty: medium
tags: transfer, heap, realloc
id: c-memory/transfer-growing-list
mode: stdout
---
This lesson gives you a description and a nearly empty file. There is no function to fill in, no `main`, and no list of cases. Everything you need is something this course or the one before it already taught: reading with `scanf`, a block from `malloc`/`realloc`, and `free`.

## Work through the recipe

1. **Understand the problem.** What goes in, what comes out, and what is *not* known in advance?
2. **Write examples.** Work out two or three by hand, including an empty input and an awkward one for the average rule below.
3. **Choose signatures.** Decide which functions you want (reading into a growing array is a natural one) and what each takes and returns before writing the bodies.
4. **Plan.** This is a **sentinel loop** that fills a growing array, then a **filter**-style pass that counts the values meeting a condition.
5. **Code.** One function at a time.
6. **Test.** Run your own inputs, including a long one. When something is wrong, state a guess ("I think the array is too small here") and print the value that proves or disproves it.

## Why the size matters

You cannot know how many numbers will arrive. A fixed array of 100 works until the 101st number, and then the program reads and writes memory it does not own. Grow the block with `realloc` when it fills, and keep the *count* separate from the *capacity*, as you did with the growable array earlier in the course.

## The average without decimals

"Greater than the average" means greater than the exact average, not the average rounded down. You can compare without any decimals: with `n` numbers adding up to `sum`, a value `x` is above the average exactly when `x * n > sum`. Use `long long` for `sum` and for that product so a long series of large numbers cannot overflow.

Hints are available. Using them is recorded, as in every lesson.
