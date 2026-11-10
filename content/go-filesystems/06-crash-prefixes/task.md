Implement `UnsafeCuts(initial State, writes []Write) []int`.

`State` is the bitmap `Used` (a slice of bools, one per block) and the inode pointer `Ptr` (a block number, or `-1` for empty). A `Write` has a `Kind`:

- `"bit"`: set `Used[Block]` to `Value == 1` (`Value` is 0 or 1);
- `"ptr"`: set `Ptr` to `Value` (a valid block or `-1`); `Block` is ignored.

Inputs obey these ranges and `initial` is itself consistent (its pointer is `-1` or names a used block).

A cut `k` (for `k` in `0 .. len(writes)`) is the state after applying the first `k` writes in order. Return, in increasing order, every cut whose state has `Ptr >= 0` and `Used[Ptr] == false`. Cut `0` is the initial state and cut `len(writes)` is the final one. Orphans (used blocks nobody points to) are allowed. If no cut is unsafe, return an empty or `nil` slice.

Do not modify `initial.Used` or `writes`. Tests use several bitmap sizes and write lists, including pointers that move several times, and compare against a brute-force replay.
