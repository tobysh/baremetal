`SIZE` is 3 and the grids are `SIZE` by `SIZE`.

- `int grid_sum(int g[SIZE][SIZE])` returns the sum of all nine cells.
- `int diagonal_sum(int g[SIZE][SIZE])` returns the sum of the main diagonal (`g[0][0]`, `g[1][1]`, `g[2][2]`).
- `int row_max(int g[SIZE][SIZE], int row)` returns the largest value in the given row. `row` is between 0 and `SIZE - 1`.
- `void transpose(int g[SIZE][SIZE])` swaps every cell with its mirror across the main diagonal, in place.
