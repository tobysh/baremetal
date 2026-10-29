---
title: Tables with Nested Loops
summary: Print a rows-by-columns multiplication grid, lining the columns up with a field width, as practice for the loops you will use on two-dimensional arrays.
minutes: 15
difficulty: easy
tags: loops, output
id: c-data/multiplication-grid
mode: stdout
---
A grid has rows and columns, and a **nested loop** visits it: the outer loop picks the row, the inner loop walks along it. You met this in [Nested Loops](/learn/first-steps/nested-loops).

```c
for (int r = 1; r <= 2; r++) {
    for (int c = 1; c <= 3; c++) {
        printf("%d ", r * c);
    }
    printf("\n");         // end the row
}
```

prints

```
1 2 3 
2 4 6 
```

## Lining columns up

Numbers have different widths, so a table looks ragged. `printf` can pad a number to a fixed **field width**: `%4d` prints the number right-aligned in 4 characters.

```c
printf("%4d", 7);     // "   7"
printf("%4d", 120);   // " 120"
```

Print every cell of a row with `%4d`, with no separating space (the padding already provides one), and a newline at the end of the row.

## Your turn

The starter declares `rows` and `cols`. Print a multiplication grid in which the cell in row `r` and column `c` (both counting from 1) holds `r * c`, printed with `%4d`. Row 1 has no special treatment; it is `1 2 3 ...` times 1. Your program is also run with other values of `rows` and `cols`, so use the variables rather than fixed numbers.
