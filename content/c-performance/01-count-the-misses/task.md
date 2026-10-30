`col_sums(out)` writes into `out[c]` the sum of `grid[r][c]` over all rows `r`, for each column `c`.

- Keep reading the grid through `ld(&grid[r][c])`. Do not skip loads and do not touch the cache block.
- Change the loop order so the grid is read in memory order. Reading every element exactly once must cost no more than **256 misses**, the minimum for a cold cache.
- The sums must be right. `out` is not cleared for you, so initialise it.

Several grids are tried, with positive and negative values. The check reads the model's counters and compares the sums with a plain reference computed separately.
