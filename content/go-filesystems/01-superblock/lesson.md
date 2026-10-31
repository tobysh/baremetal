---
title: The Superblock
summary: Decode and validate the one block that describes a whole filesystem, and lay out an empty disk with it.
minutes: 25
difficulty: easy
tags: filesystem, on-disk-format, layout
id: go-filesystems/superblock
mode: harness
---
A disk is a flat array of fixed-size **blocks**. A filesystem is nothing more than an agreement about what is stored in which block. The agreement has to be written down on the disk itself, in a well-known place, so that a fresh boot can read it: block 0, the **superblock**.

In this track you build **BMFS**, a toy filesystem with 64-byte blocks. The `Device` interface, the in-memory `MemDisk` and the `Super` struct are given. All integers on disk are **big-endian**.

```
block 0   superblock
block 1   block bitmap          (lesson 2)
block 2.. inode table           (lesson 3)
then      data blocks
```

The superblock holds seven 32-bit words: magic `0x424D4653`, version `1`, `NBlocks`, `NInodes`, `BitmapStart` (always 1), `InodeStart` (always 2) and `DataStart`. Inodes are 32 bytes, so two fit in a block, and `DataStart = 2 + ceil(NInodes / 2)`. Inode slot 0 is never used, so a disk with `NInodes = 8` has seven usable inodes.

## Why validate?

A superblock read from a disk is untrusted input. A wrong magic means this is not BMFS at all. A `DataStart` that disagrees with `NInodes` would make every later lesson read the wrong blocks. `ReadSuper` must reject anything inconsistent with `ErrBadSuper` instead of handing the rest of the system a layout it cannot trust.

Limit: one bitmap block has 64 x 8 = 512 bits, so a BMFS filesystem has at most 512 blocks. That is a design simplification, not a recommendation.
