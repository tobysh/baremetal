---
title: FizzBuzz
summary: Combine loops and decisions to solve the classic FizzBuzz programming puzzle.
minutes: 15
difficulty: medium
id: first-steps/fizzbuzz
mode: stdout
---
**FizzBuzz** is a famous little puzzle — it's been used in job interviews for years because it checks that you can combine a loop with decisions.

The rules: count from 1 to some number. For each number:

- if it's divisible by 3 **and** 5, print `FizzBuzz`
- otherwise, if it's divisible by 3, print `Fizz`
- otherwise, if it's divisible by 5, print `Buzz`
- otherwise, print the number itself

## "Divisible by"

A number `n` is divisible by 3 when there's nothing left over: `n % 3 == 0`.

## Order matters

`if / else if` checks conditions top to bottom and stops at the first true one. 15 is divisible by 3, so if you check "divisible by 3" first you'll print `Fizz` and never reach `FizzBuzz`. Think about which check has to come first.

## Printing text vs numbers

```c
printf("Fizz\n");     // text
printf("%d\n", n);    // a number
```
