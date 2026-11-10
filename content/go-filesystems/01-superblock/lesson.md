---
title: The Superblock
summary: Decode and validate the one block that describes a whole filesystem, and lay out an empty disk with it.
minutes: 25
difficulty: easy
tags: filesystem, on-disk-format, layout
id: go-filesystems/superblock
mode: harness
---
A disk is a flat array of fixed-size **blocks**. It has no idea what a file is. A filesystem is an agreement about which block holds what, and that agreement has to be written on the disk itself, at a place a freshly booted machine can find without any other information. That place is block 0, the **superblock**.

In this track you build **BMFS**, a toy filesystem on 64-byte blocks. The `Device` interface, the in-memory `MemDisk` (which counts its writes) and the `Super` struct are given. Every integer on disk is a **big-endian** 32-bit word, so `0x424D4653` is stored as the bytes `42 4D 46 53`, which spell `BMFS`.

```text
block 0     superblock
block 1     block bitmap            (lesson 2)
block 2..   inode table, 2 inodes per block   (lesson 3)
then        data blocks             up to NBlocks-1
```

The superblock holds seven words, then zero padding up to 64 bytes:

```text
word  0     1     2        3       4            5           6
      magic version NBlocks NInodes BitmapStart InodeStart  DataStart
```

The magic is `0x424D4653` and the version is `1`. `BitmapStart` is always 1 and `InodeStart` always 2. Inodes are 32 bytes, so `DataStart = 2 + ceil(NInodes / 2)`. Inode 0 is never used, so `NInodes = 8` gives seven usable inodes.

## A worked example

`FormatSuper(d, 32, 8)` on a 32-block disk needs 4 inode blocks, so `DataStart = 6`. Blocks 0 to 5 are metadata; blocks 6 to 31 are data. Block 0 holds:

```text
42 4D 46 53  00 00 00 01  00 00 00 20  00 00 00 08
00 00 00 01  00 00 00 02  00 00 00 06  00 00 00 00 ...
```

Reading a word back is one call. `encoding/binary` does the byte order for you:

```go
magic := binary.BigEndian.Uint32(b[0:4]) // 0x424D4653
nblocks := binary.BigEndian.Uint32(b[8:12])
```

The starter already wraps these as `u32(b[8:])` and `putU32(b[8:], v)`.

## Why validate?

Bytes read from a disk are untrusted input. The disk might not be BMFS at all (wrong magic), might come from a newer version, might have a bit flipped, or might be truncated so that `NBlocks` is larger than the device. If `DataStart` disagrees with `NInodes`, every later lesson would read the wrong blocks. So `ReadSuper` checks **every** field it can and returns `ErrBadSuper` rather than hand the rest of the system a layout it cannot trust. Do the checks before you use a field: for example, check `NInodes` before computing `inodeBlocks(NInodes)`.

Formatting has the mirror-image rule: validate first, then write. A `FormatSuper` that fails half way would leave a disk that is neither the old thing nor a valid new one, so a rejected call must not write at all.

## Limits of the model

One bitmap block has 64 x 8 = 512 bits, so a BMFS filesystem has at most 512 blocks. There is no checksum on the superblock and no backup copy; real filesystems keep both, because a damaged block 0 otherwise loses everything. This is a design simplification, not a recommendation.
