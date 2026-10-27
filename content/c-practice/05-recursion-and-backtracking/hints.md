`sum_digits`: if `n < 10`, return `n`. Otherwise return `n % 10 + sum_digits(n / 10)`.
===
`count_subsets`: write a helper with an extra `index` and `remaining`. At each index you either take `a[index]` (subtract it from `remaining`) or leave it, and you add the two counts. At `index == n`, return `1` if `remaining == 0`, else `0`.
===
`count_queens`: keep an array `int cols[8]`. A helper `place(n, row, cols)` tries every column `c` in this row. A column is safe if, for every earlier row `r`, `cols[r] != c` and `abs(cols[r] - c) != row - r`. When `row == n`, return `1`. Add up the results of the recursive calls. Include `<stdlib.h>` for `abs`.
