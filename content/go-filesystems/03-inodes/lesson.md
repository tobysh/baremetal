---
title: Inodes and File Contents
summary: Store each file as an inode that lists its blocks; write and read contents, and never leak a block on failure.
minutes: 45
difficulty: medium
tags: filesystem, inode, files
id: go-filesystems/inodes
mode: harness
---
A file's name is not part of the file. What describes a file is its **inode**: a small fixed-size record with the file type (free, file or directory), a link count, the size in bytes, and the numbers of the data blocks that hold the contents. Names come in lesson 4. BMFS inodes are 32 bytes:

```text
byte 0      Type  (0 free, 1 file, 2 directory)
byte 1      Links
bytes 2-3   zero
bytes 4-7   Size in bytes (big-endian)
bytes 8-31  Direct[0..5], six 4-byte block numbers
```

Six direct blocks of 64 bytes give a maximum file size of 384 bytes (`MaxFile`). Real filesystems add indirect blocks to go beyond that. Inode number `n` lives in block `InodeStart + n/2` at byte offset `(n%2)*32`, which the given helper `inodePos` computes. Because two inodes share a block, updating one means read the block, change your 32 bytes, write the block back, and the neighbour must come out unchanged.

## A worked example

A 100-byte file needs `ceil(100/64) = 2` blocks. Suppose they are blocks 9 and 10:

```text
inode 3:  Type=1 Links=1 Size=100 Direct=[9 10 0 0 0 0]
block 9 : bytes 0..63 of the file
block 10: bytes 64..99, then 28 zero bytes of padding
```

`Size` is what tells `ReadFile` where the data stops, so the padding never leaks into the result. An empty file has no blocks at all, and its `Direct` entries stay 0. In Go the number of blocks is `(len(data) + BlockSize - 1) / BlockSize`.

## The order of writes inside WriteFile

`WriteFile` replaces a file's whole contents. Do the steps in this order; the order is part of the specification and the tests record it:

1. allocate all the new data blocks (one bitmap write each);
2. write the new data blocks, zero-padded to a whole block;
3. write the inode with the new `Direct` list and `Size`;
4. free the old data blocks.

The new blocks are complete before the inode points at them, and the old ones are released only after the inode no longer does. The inode write is the single switch from old contents to new. The price is extra space: old and new copies exist together, so rewriting a large file on a nearly full disk can fail with `ErrNoSpace` even though the final result would fit.

## Failing without leaking

If the data is over 384 bytes (`ErrTooBig`) or the new blocks do not fit (`ErrNoSpace`), the file must read back exactly as before, and **no block may stay allocated** that was not allocated at the start. A leaked block is lost space that only a checker (lesson 5) can find. If you allocate block by block, give back what you already took when a later allocation fails, or check `CountFree` first.

Use the given `writeData(dev, block, bytes)` for data blocks rather than `dev.WriteBlock`; a later lesson relies on it. `AllocInode` and `ReadFile` are given.

## Limits of the model

No timestamps, permissions or sparse files; a file is always whole-file rewritten; and the four steps are not atomic as a group, which lessons 6 to 8 examine.
