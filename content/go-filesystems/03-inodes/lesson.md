---
title: Inodes and File Contents
summary: Store each file as an inode that lists its blocks; write and read contents, and never leak a block on failure.
minutes: 45
difficulty: medium
tags: filesystem, inode, files
id: go-filesystems/inodes
mode: harness
---
A file's name is not part of the file. What describes a file is its **inode**: a small fixed-size record with the type (free, file or directory), a link count, the size in bytes, and the numbers of the data blocks that hold the contents. BMFS inodes are 32 bytes:

```
byte 0 Type   byte 1 Links   bytes 2-3 zero   bytes 4-7 Size
bytes 8..31   Direct[0..5], six 4-byte block numbers
```

Six direct blocks of 64 bytes give a maximum file size of 384 bytes. Inode number `n` lives in block `2 + n/2` at byte offset `(n%2)*32`. Because two inodes share a block, writing one means read-modify-write of the block, and the neighbour must come out unchanged.

## The order of writes inside WriteFile

`WriteFile` replaces a file's contents. The steps, in this order, are part of the specification:

1. allocate all the new data blocks (one bitmap write each);
2. write the new data blocks, zero-padded to a whole block;
3. write the inode with the new `Direct` list and `Size`;
4. free the old data blocks.

The new blocks are written before the inode points at them, and the old ones are released only after the inode no longer does. That makes the replacement a single switch (the inode write). It costs extra space during the rewrite: the old and new copies exist together, so rewriting a big file on a nearly full disk can fail with `ErrNoSpace` even though the final result would fit. If that happens, or the data is over 384 bytes (`ErrTooBig`), the file must be unchanged and no block may stay allocated.

`AllocInode` and `ReadFile` are given.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
