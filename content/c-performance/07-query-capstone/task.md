Implement `build_table`, `query` and `table_free`.

- `build_table(rows, n)` makes one `sys_alloc` of exactly `2 * round64(4n) + round64(4 * (REGIONS + 1))` bytes and fills `price` and `qty` stable-sorted by region (rows keep their input order inside a region) and `start[0..REGIONS]`. Regions in the input are `0..REGIONS-1`. `n` may be 0.
- `query(t, region, min_price)` returns the sum of `price * qty` as a `long` for rows of that region with `price >= min_price`. A region outside `0..REGIONS-1` returns 0 without reading `start`.
- Read the columns only through `ld()`. Read `qty` only for rows whose price qualifies.
- `table_free(t)` releases the block (`sys_live` back to 0).
- A query must stay under `2 * (k/16 + 2) + 2` cache misses from a cold cache, where `k` is the number of rows in the region.

Do not edit the cache block, `sys_alloc`, `sys_free` or `query_aos`.
