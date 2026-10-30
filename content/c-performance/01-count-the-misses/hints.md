Run the program and read the two numbers. 4096 accesses and 4096 misses means every single access was a miss.
===
`grid[r][c]` and `grid[r][c+1]` are neighbours in memory; `grid[r][c]` and `grid[r+1][c]` are not. Which index changes fastest in your inner loop?
===
Make `r` the outer loop and `c` the inner loop, and add into `out[c]`. Set every `out[c]` to 0 before the loops.
