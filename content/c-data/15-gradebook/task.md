Write a gradebook. The input starts with the number of students (at most 20), followed by one line per student holding a one-word name (at most 15 characters) and an integer score.

The program prints a ranking, the average, and how many students passed. Ranking lines read `RANK. NAME SCORE`, from the highest score to the lowest, numbered from 1. Students with equal scores keep the order they were entered in. The average is printed with two decimals. A score of 50 or more is a pass. For the input `3`, `ada 91`, `grace 78`, `linus 91` the output is:

```
1. ada 91
2. linus 91
3. grace 78
average: 86.67
passed: 3 of 3
```

An empty class has no ranking and no one to average, so it prints just `average: 0.00` and `passed: 0 of 0`.

Other classes are tried. Start from the empty file: choose your own struct, functions and `main`.
