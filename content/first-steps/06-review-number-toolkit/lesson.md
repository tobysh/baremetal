---
title: "Review: Number Toolkit"
summary: A relaxed consolidation exercise. Combine functions, if statements and loops in three small number helpers. No new ideas, just practice.
minutes: 15
difficulty: easy
kind: review
tags: functions, conditionals, loops, review
id: first-steps/review-number-toolkit
mode: harness
---
This is a **review lesson**. It introduces nothing new. You have now met functions, `if` and loops, and the best way to make them stick is to use them together.

## What you already know

- **Functions:** take inputs, do work, `return` a result.
- **Decisions:** `if` and comparison operators choose what to do, and `%` gives the remainder after division.
- **Loops:** `for` repeats work, and the classic mistake is being off by one at either end.

## The three helpers

1. **`digit_sum`** adds up the digits of a number. Peel digits off the end: `n % 10` is the last digit, and `n / 10` is the number without it. Loop until nothing is left.
2. **`count_divisible`** counts how many numbers in a range divide evenly by something. That is a loop with an `if` inside, and `%` is the test: `x % d == 0`.
3. **`is_prime`** asks whether a number has any divisor other than 1 and itself. Try every candidate divisor from 2 upwards and bail out with an early `return 0` if one fits.

Nothing here is a trick. If you get stuck, reread the lesson the idea came from, or open the hints one at a time.
