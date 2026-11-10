---
title: Order a Safe File Replacement
summary: Publish initialized data before freeing the previous file blocks.
minutes: 40
difficulty: medium
tags: filesystem, crash-consistency, write-ordering
id: go-filesystems/ordered-replacement
---
Lesson 6 showed that the order of writes decides which crash states can exist. This lesson picks the right order for the most common operation of all: replacing the contents of a file. It is the one-block version of what `WriteFile` did in lesson 3.

## Two ways to get it wrong

The obvious plan is "overwrite the old block in place". A crash in the middle of an in-place rewrite of a multi-block file leaves a mixture of old and new data, and the filesystem has no way to know. The next obvious plan is "free the old block first to make room". A crash right after the free leaves the inode pointing at a block the allocator considers free, which another file can claim at any moment. Both lose data that was safe until we touched it.

The safe plan never modifies anything that the inode currently points at until the switch has been made:

```text
1. alloc  fresh        reserve a new block in the bitmap
2. data   fresh        write the new contents into it
3. inode  -> fresh     switch the inode pointer (the single commit point)
4. free   old          release the old block
```

## What each crash cut looks like

```text
cut 0  ptr=old    old used, fresh free             nothing happened
cut 1  ptr=old    old used, fresh used (orphan)    leaked block, file intact
cut 2  ptr=old    fresh holds data but unreferenced  leaked block, file intact
cut 3  ptr=fresh  both used, old is an orphan      new contents live, old block leaked
cut 4  ptr=fresh  old free                         done
```

At every cut the pointer names a block that is both **allocated** and **initialized with the right data**. The file is entirely old (cuts 0 to 2) or entirely new (cuts 3 and 4); there is no in-between. The cost is that a crash may leak one block, which a checker (lesson 5) can find and free later. Leaking is the acceptable failure; pointing at free or uninitialized storage is not.

The same idea appears everywhere: write a temporary file, flush it, then `rename` it over the original; the rename is the commit point. The rule is "make the new state complete before anything points at it, and release the old state only after nothing points at it".

## The task

`Replacement(old, fresh, value)` returns the four writes. The write type is:

```go
type Write struct {
	Kind         string // "alloc", "data", "inode" or "free"
	Block, Value int
}
```

A worked example with `old=3`, `fresh=7`, `value=42` is `{alloc 7 0} {data 7 42} {inode 7 0} {free 3 0}`. `Value` carries the contents only for the data write; the other writes carry 0.

## Limits of the model

It assumes atomic ordered writes, so a real disk also needs a flush or barrier between steps 2 and 3, otherwise the device may reorder them. One block per file; a many-block file would repeat steps 1 and 2 per block and still publish with one inode write.
