---
title: The Block Bitmap
summary: Track which blocks are free with one bit each; allocate the lowest free block and refuse double frees.
minutes: 30
difficulty: easy
tags: filesystem, bitmap, allocation
id: go-filesystems/block-bitmap
mode: harness
---
To store a file you need to know which blocks are free. The simplest answer is a **bitmap**: block 1 holds one bit per block of the filesystem, `1` meaning in use. Bit `b` is byte `b/8`, position `b%8`, least-significant bit first.

The metadata blocks (superblock, bitmap, inode table) are in use from the start, so `MarkMetadata` sets those bits right after formatting. After that a data block can only be handed out by `AllocBlock`, which takes the **lowest** free data block, and given back by `FreeBlock`.

## Count the writes

Every bitmap change is exactly **one block write**: read block 1, change a bit, write block 1. This looks like a detail now. In lessons 6 to 8 the number and order of these writes is what a crash can cut in half, so the tests count them and so should you.

## Mistakes the allocator must refuse

- freeing a metadata block (that would let the allocator hand out the superblock);
- freeing a block twice (two files could then get the same block);
- allocating past `NBlocks`, even when the device is bigger than the filesystem.

A failed call must not write.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
