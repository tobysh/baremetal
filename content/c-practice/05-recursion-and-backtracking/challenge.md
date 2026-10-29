**Challenge: queens on a damaged board.**

Some squares of the board have been cut out. Write

```c
int count_queens_on(int n, const char *const *board);
```

`board` is `n` strings of `n` characters each, one per row, top row first. A `'.'` is a normal square and a `'#'` is a hole: no queen may stand on a hole. Return how many ways there are to place **`n` queens** on normal squares so that no two share a row, a column or a diagonal. (A queen is not blocked by a hole: a queen can still attack along a line through a hole.)

- `n` is between `1` and `9`.
- Place one queen per row and backtrack, keeping track of which columns and diagonals are taken. The counts for an undamaged board are the ones from the lesson (`n = 8` is `92`), but the tests use many different boards, so only a real search will do.
- Do not modify the board.
