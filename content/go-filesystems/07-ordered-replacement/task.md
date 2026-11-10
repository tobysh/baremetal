Implement `Replacement(old, fresh, value int) []Write`.

`old` and `fresh` are distinct non-negative block numbers. Initially `old` is allocated, holds initialized data and is the block the inode points at; `fresh` is free. `value` is any integer, negative and zero included, and stands for the new contents of the block.

Return exactly four writes, in this order, as a new slice each call:

1. `Kind: "alloc"`, `Block: fresh`: reserve the new block;
2. `Kind: "data"`, `Block: fresh`, `Value: value`: write its contents;
3. `Kind: "inode"`, `Block: fresh`: point the inode at it;
4. `Kind: "free"`, `Block: old`: release the old block.

`Value` is `value` for the data write only and `0` for the other three. The hidden tests replay every prefix of the writes (cut after 0, 1, 2, 3 and 4 writes) for many block numbers and values, and check that the inode always names allocated, initialized data, that the file is entirely old before the inode write and entirely new after it, and that the final state is exactly the new file.
