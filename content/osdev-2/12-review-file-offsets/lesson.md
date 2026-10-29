---
title: Review: File Offsets and Bounded Storage
summary: Combine bounded storage and descriptor offsets in a small file table.
minutes: 35
difficulty: medium
tags: kernel, systems
id: osdev-2/review-file-offsets
mode: harness
kind: review
---
A file descriptor refers to an open file description, which carries the current offset. Two independently opened descriptors may point to the same file but keep different offsets. This review combines the bounds checks from the [RAM disk](/learn/osdev-2/ramdisk) with the state transitions from [pipes](/learn/osdev-2/pipes).

This model has one fixed 32-byte file and four independent descriptor slots. Opening chooses a free slot, reading advances only that slot, and closing makes the slot available again. Real kernels add permissions, sharing after `dup` or `fork`, filesystem metadata and locking.
