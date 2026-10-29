---
title: Two-Dimensional Arrays
summary: Store a grid as an array of arrays, index it with grid[row][col], and write functions that sum, scan and transpose it.
minutes: 20
difficulty: medium
tags: arrays, loops
id: c-data/two-dimensional-arrays
mode: harness
---
A grid is just an array whose elements are themselves arrays. Declare it with two sizes and index it with two brackets, row first:

```c
int grid[2][3] = {
    {1, 2, 3},
    {4, 5, 6},
};

printf("%d\n", grid[1][2]);   // 6: row 1, column 2
grid[0][0] = 10;
```

Visit every cell with a nested loop (the shape from [the previous lesson](/learn/c-data/multiplication-grid)):

```c
for (int r = 0; r < 2; r++) {
    for (int c = 0; c < 3; c++) {
        total += grid[r][c];
    }
}
```

## Passing a grid to a function

For a 2D array parameter, C needs every size except the first. The program defines `SIZE` with `#define`, a name that is replaced by `3` before compiling:

```c
#define SIZE 3

int grid_sum(int g[SIZE][SIZE]);
```

Like one-dimensional arrays, the function works on the original grid, so it can change it.

## Transposing

Transposing mirrors a square grid across its diagonal: the cell at row `r`, column `c` swaps with the cell at row `c`, column `r`. Swap each pair only **once**. If you visit every cell you will swap each pair twice and end up where you began. Visiting only the cells with `c > r` (above the diagonal) does it.

## Your turn

The starter defines `SIZE` as 3. Write the four functions below. The tests build several different grids.
