Run the program: `query_aos` takes about 2000 misses for 2000 rows even though only about a quarter of them are in the region.
===
Do a counting sort: `start[r+1]` first holds the count of region `r`, then prefix-sum, then place each row at a moving cursor per region (copy of `start`) so the order inside a region is preserved.
===
Carve the columns out of one block: `price = block`, `qty = block + round64(4n)` bytes later, `start = block + 2*round64(4n)` bytes. Query: loop `i` from `start[r]` to `start[r+1]`, `ld(&price[i])`, and only if it is at least `min_price` add `(long)price * ld(&qty[i])`.
