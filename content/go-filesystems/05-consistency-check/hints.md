Count first, classify second. Make `counts := make([]int, NBlocks)` and add one for every pointer that lies in `[DataStart, NBlocks)`. Pointers outside that range never index `counts`; they go into `Invalid`.
===
Validate geometry at the very top: `NBlocks < 0`, `DataStart < 0`, `DataStart > NBlocks`, or `len(Used) != NBlocks` all return `Report{BadGeometry: true}`. Do this before `make([]int, NBlocks)`, which would panic on a negative size.
===
Then loop `b` over `DataStart .. NBlocks-1` once and append to `Orphan`, `Dangling` and `Duplicate` as the three conditions allow. Because you walk upward, those lists come out ascending and unique without any sorting. Only `Invalid` needs `sort.Ints`.
===
Watch the edge cases the tests use: a pointer repeated inside one inode is two references (a Duplicate), an invalid pointer repeated twice appears twice in `Invalid`, and a block can be Dangling and Duplicate at the same time.
