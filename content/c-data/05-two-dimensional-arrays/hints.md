`grid_sum` is two nested loops over `r` and `c` from 0 to `SIZE - 1`, adding `g[r][c]`. `diagonal_sum` needs only one loop, because the diagonal cells are `g[i][i]`.
===
`row_max` is the array-maximum loop on `g[row][c]`, starting from `g[row][0]` (values may be negative). For `transpose`, loop `r` over all rows but `c` from `r + 1`: that visits only the cells above the diagonal, so each pair is swapped once.
===
```c
void transpose(int g[SIZE][SIZE]) {
    for (int r = 0; r < SIZE; r++) {
        for (int c = r + 1; c < SIZE; c++) {
            int tmp = g[r][c];
            g[r][c] = g[c][r];
            g[c][r] = tmp;
        }
    }
}
```
