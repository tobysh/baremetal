---
title: Loops inside loops
summary: Put one loop inside another to print patterns and tables.
minutes: 20
difficulty: medium
tags: loops
id: first-steps/nested-loops
mode: stdout
---
A loop body can contain anything, including another loop. The **inner** loop runs completely, from start to finish, for *every single step* of the **outer** loop.

```c
for (int row = 1; row <= 3; row++) {
    for (int col = 1; col <= row; col++) {
        printf("#");
    }
    printf("\n");
}
```

prints

```
#
##
###
```

Trace it: when `row` is 1 the inner loop runs once (one `#`); when `row` is 2 it runs twice; when `row` is 3, three times. After each inner loop finishes, `printf("\n")` ends the line.

## Rows and columns

Think of the outer loop as the **rows** and the inner loop as the **columns** in each row. A table of `row * col` works the same way:

```c
for (int row = 1; row <= 2; row++) {
    for (int col = 1; col <= 3; col++) {
        printf("%d ", row * col);
    }
    printf("\n");
}
```

## No trailing spaces

The loop above leaves a space after the last number on every line. Tests compare your output exactly, so print the separator only *between* items: print a space before every column except the first (`if (col > 1) printf(" ");`).

## Name them well

`i` and `j` are fine, but `row` and `col` make nested loops much easier to read and to debug.
