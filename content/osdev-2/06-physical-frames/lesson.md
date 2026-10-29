---
title: Physical Frame Allocation
summary: Track available physical page frames with a bitmap and reserve ranges safely.
minutes: 35
difficulty: medium
tags: kernel, systems
id: osdev-2/physical-frames
mode: harness
---
An address space needs physical frames before it can build page tables. A bitmap uses one bit per frame: a set bit means unavailable, whether allocated or reserved. Here the caller supplies an already-discovered range of 64 frames; discovering usable memory from a bootloader map is a separate job.

Allocation scans for a free bit and marks it before returning the corresponding address. Address zero is reserved as the failure sentinel, so this pool starts at 0x100000. Freeing must validate alignment and range before calculating a bit index. A reservation marks each frame in a half-open index range and is idempotent. On a multiprocessor this state needs a lock; this exercise assumes one caller at a time.
