---
title: Find Orphans and Duplicate Owners
summary: Audit a block ownership graph without trusting inode pointers or bitmap state.
minutes: 40
difficulty: hard
tags: filesystem, fsck, consistency
id: go-filesystems/consistency-check
---
A filesystem keeps the same fact in two places. The **bitmap** says which blocks are in use; the **inodes** say which blocks each file owns. They are updated by separate writes, so a crash, a bug or a failing disk can make them disagree. A consistency checker (`fsck`) is the tool that compares them. Lesson 6 shows how a crash creates the disagreements; this lesson is the audit that finds them.

## The four findings

For each data block, count how many inode pointers name it and compare with its bitmap bit:

```text
references  bitmap   verdict
0           used     Orphan     allocated but nobody owns it: leaked space
>= 1        free     Dangling   owned but marked free: the allocator may reuse it
>= 2        any      Duplicate  two owners (or one inode listing it twice)
0           free     fine
1           used     fine
```

A pointer outside the data region `[DataStart, NBlocks)` is **Invalid**: it could point into the superblock, the inode table, or past the end of the disk. Orphan and Dangling are the two halves of "the bitmap is wrong"; Duplicate and Invalid mean "an inode is wrong", which is more serious.

## A worked example

```text
NBlocks=10  DataStart=3
Used:   0 1 2 3 4 5 6 7 8 9
        T T T T T F T F F F
inode A: [3, 4]
inode B: [4, 5, 12]
```

Block 3 has one owner and is marked used: fine. Block 4 has two owners: **Duplicate**. Block 5 is owned but free: **Dangling**. Block 6 is used but nobody points at it: **Orphan**. Pointer 12 is past the end: **Invalid**. Bits 0 to 2 are metadata and are not examined. The report is `Orphan [6]`, `Dangling [5]`, `Duplicate [4]`, `Invalid [12]`.

The core is a reference count, built before any classification so the order of inodes does not matter:

```go
counts := make([]int, im.NBlocks)
for _, inode := range im.Inodes {
	for _, b := range inode {
		if b >= im.DataStart && b < im.NBlocks {
			counts[b]++
		}
	}
}
```

## Do not trust the input

The image is the thing being audited, so it may be garbage. Check the geometry (`0 <= DataStart <= NBlocks`, and `len(Used) == NBlocks`) **before** indexing anything, and report `BadGeometry` instead of panicking. Pointers can be negative or enormous; they must be range-checked before they index `counts`. A pointer that repeats (`[9, 9]`) counts twice, both as a duplicate reference and, if invalid, as two Invalid findings.

## Report, do not repair

The checker only reports. Automatic repair is dangerous: "free the duplicate block" frees a block that another file is still using, and "mark dangling blocks used" may bless a pointer that was corrupted. Real tools show evidence first and let a policy or a person choose. The checker also must not mutate the image; the same image should be checkable twice with the same answer.

## Limits of the model

The image is already decoded (`Inodes` is a list of block lists), so there is no parsing here. Directory trees, link counts and sizes are not checked, and there is no notion of which of several owners is the "real" one.
