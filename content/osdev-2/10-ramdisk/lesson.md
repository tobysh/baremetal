---
title: A RAM Disk with Fixed Blocks
summary: Read and write bounded byte ranges on a memory-backed block device.
minutes: 35
difficulty: medium
tags: kernel, systems
id: osdev-2/ramdisk
mode: harness
---
A RAM disk exposes an array of bytes through a device interface. Filesystems build their own metadata above this layer; the device itself knows only offsets and lengths. That separation lets the same filesystem use a disk driver later.

Validate the entire range before copying so a failed request cannot partially modify the device. The subtraction form `len <= size - offset` avoids overflow once the offset has been checked. Zero-length operations at the end are valid.
