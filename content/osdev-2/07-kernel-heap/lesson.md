---
title: A Small Kernel Heap
summary: Allocate aligned blocks from a fixed arena and coalesce neighbours on free.
minutes: 35
difficulty: medium
tags: kernel, systems
id: osdev-2/kernel-heap
mode: harness
---
Physical frames are too coarse for small kernel objects. A heap divides an arena into blocks and tracks whether each block is in use. This exercise models block metadata separately so you can focus on splitting and coalescing; a production allocator also handles concurrency, corrupted metadata and returning pages to the frame allocator.

Round requests up to eight bytes, split a free block only when there is room for another descriptor, and merge adjacent free blocks on release. Do not merge non-neighbours or accept a pointer into the middle of a block. The arena and descriptors are reset between tests.
