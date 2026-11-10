---
title: Enumerate Every Crash Point
summary: Replay prefixes of atomic block writes and identify inconsistent durable states.
minutes: 40
difficulty: hard
tags: filesystem, crash-consistency, enumeration
id: go-filesystems/crash-prefixes
---
Power can fail between any two writes. An operation such as "give this file a new block" is several block writes, and the disk keeps only the ones that finished. Testing by pulling the plug at random finds bugs by luck. This lesson replaces luck with a complete enumeration: replay the writes one at a time and check the state after **every** possible stopping point.

## The crash model

Our model is deliberately simple. Each write is a whole block, applied atomically, and writes reach the disk in the order issued. A crash after `k` writes therefore leaves exactly the first `k` writes applied, for every `k` from `0` (nothing survived) to `n` (everything did). That is `n + 1` possible durable states for `n` writes. Real disks can also tear a block in half, reorder writes, or hold them in a volatile cache; none of that is modelled.

## The state we watch

To keep the focus on ordering, the state is tiny: a bitmap `Used` and one inode pointer `Ptr` naming a block (`-1` means the inode points nowhere). A write is either

```go
Write{"bit", block, 0 or 1}  // set or clear one bitmap bit
Write{"ptr", _, value}       // inode now points at block `value` (or -1); Block is ignored
```

A durable state is **unsafe** when `Ptr >= 0` and `Used[Ptr]` is false: the inode points at a block the allocator thinks is free. The allocator may hand it to another file, and two files then overwrite each other. The reverse, a block marked used that no inode points at (an **orphan**), only wastes space, and a checker like the one in lesson 5 can reclaim it. The model therefore treats orphans as allowed.

## A worked example

Start with `Used = [T, F, F]`, `Ptr = 0`, and move the file from block 0 to block 1. Two orderings:

```text
safe:    1. bit 1 = 1    2. ptr = 1    3. bit 0 = 0
cut 0: ptr 0, bit0 T       ok
cut 1: ptr 0, bit1 T       ok   (block 1 is an orphan)
cut 2: ptr 1, bit1 T       ok
cut 3: ptr 1, bit0 F       ok            -> unsafe cuts: none

unsafe:  1. bit 0 = 0    2. ptr = 1    3. bit 1 = 1
cut 1: ptr 0, bit0 F       UNSAFE
cut 2: ptr 1, bit1 F       UNSAFE
cut 3: ptr 1, bit1 T       ok            -> unsafe cuts: 1, 2
```

The rule the safe order follows: **allocate before you point, point before you free**. Every state in between holds only orphans, never dangling pointers.

## The algorithm

Copy the initial bitmap (the caller's state is not yours to change), then walk `k = 0 .. len(writes)`: test the current state, and only then apply write `k` if there is one. Cut `0` and cut `n` both count, and the result is in increasing order. Notice that a write list can change `Ptr` several times and set and clear the same bit repeatedly; only the replay tells you the state at each cut.

## Limits of the model

Only one pointer and atomic whole-block writes. Real crash testing tools (and the next two lessons) use the same idea, enumerate the prefixes of the write log, but with real block images and recovery code run after each cut.
