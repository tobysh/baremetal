---
title: Recover short directory entries
summary: Decode synthetic FAT short-name directory records and mark deleted entries without inventing lost names.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/fat-directory
mode: harness
---
A FAT short-name directory uses 32-byte records. Byte zero is special: 0x00 ends the directory, while 0xe5 marks a deleted entry. Deleted entries may still contain useful size and cluster fields, but the first character of the original name is lost. Represent it as `_` rather than guessing.

Bytes 0..7 contain a space-padded base name, bytes 8..10 a space-padded extension and byte 11 attributes. Skip long-file-name entries (attribute 0x0f) and volume labels (bit 0x08); retain directories and mark them using bit 0x10. Trim trailing spaces from each name part and insert a dot only when an extension remains.

Combine the high cluster word at offset 20 and low word at offset 26, then read the 32-bit size at offset 28, all little-endian. Process only complete records and stop once the output array is full. This lesson lists metadata only; it does not traverse allocation chains or recover contents. A deleted entry's old clusters might already belong to another file, so its metadata alone cannot establish what was recovered.
