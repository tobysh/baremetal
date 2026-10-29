---
title: Read an MBR
summary: Decode the four little-endian partition slots in a synthetic 512-byte master boot record.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/partition-tables
mode: harness
---
The legacy master boot record contains four 16-byte partition slots starting at offset 446 and a `55 aa` signature at offsets 510 and 511. This exercise receives exactly 512 bytes, so the sector length is a caller guarantee. Reject a swapped or missing signature.

Within each slot, byte 0 is the boot indicator, byte 4 the partition type, bytes 8..11 the starting LBA and bytes 12..15 the sector count. The two numbers are little-endian: combine bytes with explicit unsigned shifts. Ignore slots whose type or sector count is zero and compact the others into output order. Bootable means the indicator equals 0x80.

The type byte is only a label. Use the names listed in the task and report unrecognized values as Unknown. A GPT protective MBR does not describe all GPT partitions, an extended partition is not the complete logical-partition chain, and partition bounds are not validated against any actual disk size here. Documenting those limits prevents a small decoder from presenting itself as a complete disk validator.
