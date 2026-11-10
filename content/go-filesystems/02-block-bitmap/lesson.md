---
title: The Block Bitmap
summary: Track which blocks are free with one bit each; allocate the lowest free block and refuse double frees.
minutes: 30
difficulty: easy
tags: filesystem, bitmap, allocation
id: go-filesystems/block-bitmap
mode: harness
---
To store a file you must know which blocks are free. Scanning every inode on every allocation would be slow and fragile, so filesystems keep a separate summary: a **bitmap** with one bit per block, `1` meaning in use. In BMFS the bitmap is block 1, and 64 bytes x 8 bits cover up to 512 blocks.

Bit `b` lives in byte `b/8` at position `b%8`, counted from the least significant bit:

```text
byte 0 = 0b00001111   blocks 0,1,2,3 in use (superblock, bitmap, 2 inode blocks)
byte 1 = 0b00000010   block 9 in use (8 + 1)
```

In Go the three operations are one-liners on a byte slice `bm`:

```go
used := bm[b/8]&(1<<uint(b%8)) != 0
bm[b/8] |= 1 << uint(b%8)  // mark used
bm[b/8] &^= 1 << uint(b%8) // mark free (AND NOT)
```

## What starts out used

Right after `FormatSuper` the bitmap block is all zero, which would let the allocator hand out the superblock. `MarkMetadata` sets bits `0 .. DataStart-1` and clears everything else, in a single write. After that only `AllocBlock` takes blocks, always the **lowest** free data block, and only `FreeBlock` gives them back. Say `DataStart` is 4 and blocks 4 and 5 are in use: `AllocBlock` returns 6. Free 4, and the next allocation returns 4 again.

## Count the writes

Every bitmap change is **one** block write: read block 1, change a bit, write block 1. That looks like a detail now. In lessons 6 to 8 the number and order of these writes is exactly what a crash can cut in half, so the tests count them with `MemDisk.Writes`, and a call that fails must not write at all.

## Mistakes the allocator must refuse

- freeing a metadata block (the allocator would later hand out the superblock);
- freeing a block that is already free (a double free is how two files end up with the same block);
- allocating at or past `NBlocks`. The device may be larger than the filesystem, and the bitmap has 512 bits even when the filesystem has 40 blocks, so never scan to the end of the bitmap. Stop at `NBlocks`.

Note that `GetBit` and `SetBit` accept any block in `0 .. NBlocks-1`, including metadata blocks, because `MarkMetadata` and later tools need them. The protection against touching metadata sits in `FreeBlock`.

## Limits of the model

One bitmap block caps the filesystem at 512 blocks, allocation is a linear scan, and there is no attempt to place a file's blocks next to each other. Real filesystems use multi-block bitmaps or extent trees to avoid all three.
