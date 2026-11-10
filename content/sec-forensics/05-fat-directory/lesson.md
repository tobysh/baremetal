---
title: Recover short directory entries
summary: Decode synthetic FAT short-name directory records and mark deleted entries without inventing lost names.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/fat-directory
mode: harness
---
## The forensic question

A user says "I never had a file called budget.xlsx". The directory of a FAT volume may still hold a record for it, marked deleted. What can you read from that record, and what can you not?

## The record format

A FAT directory is an array of 32-byte records. The fields used here:

```text
offset  size  field
0       8     base name, space padded
8       3     extension, space padded
11      1     attributes   0x0f = long-name piece, 0x08 = volume label, 0x10 = directory
20      2     cluster, high 16 bits  (little-endian)
26      2     cluster, low 16 bits
28      4     size in bytes          (little-endian)
```

The first byte is special: `00` means "no more entries", `e5` means "this entry was deleted". Deleting a file overwrites only that first byte, so the first letter of the name is lost. Here are two synthetic records in a hex-and-text sketch:

```text
4c 45 44 47 45 52 20 20 50 44 46 20 ...  "LEDGER  PDF"  size 2928  live
e5 4f 4e 54 41 43 54 53 44 4f 43 20 ...  "?ONTACTS DOC" size 4096  deleted
```

## Worked example

Decode the second record.

1. Byte 0 is `e5`: deleted. Do not stop; deleted entries are kept. The first name character is unknown, so write `_`.
2. Name bytes 1..7 are `ONTACTS` with no padding; extension bytes 8..10 are `DOC`.
3. Trim trailing spaces from each part, then join with a dot only if the extension is not empty: `_ONTACTS.DOC`.
4. Attributes are `20` (archive): not a long-name piece, not a label, not a directory.
5. Cluster = high word `<< 16` | low word; size = four bytes little-endian.

Now the first record: byte 0 is `4c`, so it is live and the name is `LEDGER.PDF`.

## Pitfalls

- **`00` ends the listing**, even if later records look valid; those are stale bytes.
- **Skip, do not decode, long-name pieces and labels.** A record with attribute `0x0f` holds name fragments, not a file. A volume label has bit `0x08`.
- **Trim spaces, then dot.** `README` with an empty extension is `README`, not `README.`. A name that fills all eight characters is `ABCDEFGH.TXT`: size the buffer for 13 characters.
- **Whole records only.** If `n` is not a multiple of 32 the last partial record is ignored. Compute remaining bytes with subtraction.
- **Combine the cluster words** with shifts on `uint32_t` values.
- **Respect `max`**; stop storing when the output is full.

## What a defender concludes

A deleted record shows that an entry with that size and starting cluster existed. It does not prove the content is recoverable: the clusters may have been reused by another file since. Report "a deleted entry named `_ONTACTS.DOC`, size 4096, starting at cluster N", never "recovered contacts.doc". This lesson reads metadata only and does not follow allocation chains.
