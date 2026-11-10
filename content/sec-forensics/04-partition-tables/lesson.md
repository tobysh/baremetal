---
title: Read an MBR
summary: Decode the four little-endian partition slots in a synthetic 512-byte master boot record.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/partition-tables
mode: harness
---
## The forensic question

A disk image arrives with no notes. Before any file system is examined, the investigator asks: how is the disk divided, which partition is bootable, where does each begin and how large is it? On legacy disks the answer lives in the first 512-byte sector, the **master boot record** (MBR).

## The layout

```text
offset  size  meaning
0       446   boot code (ignored here)
446     16    partition slot 0
462     16    partition slot 1
478     16    partition slot 2
494     16    partition slot 3
510     2     signature: 55 aa
```

Inside one 16-byte slot:

```text
byte 0       boot indicator   0x80 = bootable, anything else = not
byte 4       partition type   07, 0b, 0c, 82, 83, 05, ee, ef ...
bytes 8-11   start LBA        little-endian 32-bit
bytes 12-15  sector count     little-endian 32-bit
```

A synthetic slot in hex:

```text
80 00 00 00 07 00 00 00 00 08 00 00 00 40 06 00
```

## Worked example

1. Byte 0 is `80`: bootable.
2. Byte 4 is `07`: NTFS/exFAT by the type table (a label, not proof).
3. Bytes 8-11 are `00 08 00 00`. Little-endian means the first byte is least significant: `0x00000800 = 2048`. Build it with shifts: `b[0] | b[1]<<8 | b[2]<<16 | b[3]<<24`, casting each byte to `uint32_t` first so the shifts cannot overflow an `int`.
4. Bytes 12-15 are `00 40 06 00` = `0x00064000 = 409600` sectors, about 200 MiB at 512 bytes per sector.

So the partition starts at sector 2048 and spans 409600 sectors, ending just before sector 411648. A second slot that begins at 411648 would sit directly after it, which is a normal layout. A slot beginning at 2100 would overlap this one and deserve a note.

## Pitfalls

- **Signature first.** A sector whose bytes 510 and 511 are not `55 aa` is not a valid MBR; return -1 and decode nothing. Check both bytes: `aa 55` is a swapped signature.
- **Empty slots are all zero.** Skip a slot if its type or its sector count is zero, and compact the survivors so the output has no holes.
- **Only 0x80 means bootable.** `0x01` or `0xff` do not.
- **Shift width.** `p[3] << 24` shifts an `int`; a high byte of `0x80` or more changes the sign. Cast first.
- **Labels are not proof.** The type byte only names what the creator intended.

## What a defender concludes

An MBR tells you how the disk is meant to be divided. It does not validate that the partitions fit the physical disk, it does not describe the logical drives inside an extended partition, and a GPT protective entry (`ee`) means the real table is elsewhere. Report the entries you decoded and state those limits rather than presenting the result as a complete layout.
