---
title: "Project: A Gradebook"
summary: Read student records from input into an array of structs, find the top student, rank everyone by score with a stable sort and print a report with an average.
minutes: 30
difficulty: hard
tags: structs, arrays, strings, input, project
id: c-data/gradebook
mode: stdout
---
Time to use everything: arrays, strings and structs together, with input.

## The input

The first line is a number `n` (at most 20), the count of students. Then come `n` lines, each a name (one word, at most 15 characters) and an integer score:

```
3
ada 91
grace 78
linus 91
```

Read with `scanf`, as in [Reading Input](/learn/first-steps/reading-input). `%s` reads one word; give it a width so it cannot overflow the array: `scanf("%15s %d", s.name, &s.score)` (the `&` before the number is explained in the next course; just write it for `scanf` as before). `scanf` returns how many items it filled, which is a good loop condition.

## The report

1. Print a **ranking**: one line per student from the highest score to the lowest, formatted `RANK. NAME SCORE`, where rank is 1, 2, 3... by position. Students with equal scores keep the order they were entered in, which is what a stable sort gives you ([Arrays of Structs](/learn/c-data/arrays-of-structs)).
2. Print `average: X.XX` with two decimals, computed in `double` (the average of all scores).
3. Print `passed: P of N`, where P counts scores of 50 or more.

If `n` is 0, print only `average: 0.00` and `passed: 0 of 0`, never dividing by zero.

Break the work into small functions if you like; the shape is read, sort, print.
