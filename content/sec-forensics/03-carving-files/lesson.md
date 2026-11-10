---
title: Carve a synthetic image
summary: Locate complete JPEG-like and PNG-like byte ranges while respecting input and output bounds.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/carving-files
mode: harness
---
## The forensic question

A suspect deleted a folder of pictures and emptied the recycle bin. The directory entries are gone, but the data blocks may still sit in the disk image. Can you find the pictures without any directory to point at them? Searching raw bytes for the start and end markers of known formats is called **carving**.

## The synthetic format

This course uses two simplified object types:

```text
JPEG-like   starts  ff d8 ff
            ends    at the first ff d9 found after the 3 header bytes (marker included)
PNG-like    starts  89 50 4e 47 0d 0a 1a 0a
            ends    8 bytes after the first "IEND" after the header
                    (the 4 letters plus 4 CRC bytes)
```

Here is a tiny image, 24 bytes, offsets in decimal:

```text
offset  bytes
 0-5    20 20 20 20 20 20            padding
 6-8    ff d8 ff                     JPEG header
 9-11   11 22 33                     payload
12-13   ff d9                        JPEG footer
14-17   30 30 30 30                  padding
18-25   89 50 4e 47 0d 0a 1a 0a      PNG header (carve continues past 24 here)
```

## Worked example

Scan from offset 0 and ask at every offset whether a header starts there.

1. Offsets 0 to 5 are spaces: no header, advance one byte each.
2. Offset 6 holds `ff d8 ff`. Look for `ff d9` starting at offset 9 (after the header). It is at 12, so the object ends at 14. Report JPEG, offset 6, length 8.
3. Continue at 14, not at 7: carved ranges must not overlap.
4. The PNG header starts at 18 but if no `IEND` plus four CRC bytes follow, the object is incomplete. Report nothing and advance one byte.

The result is one JPEG at offset 6 with length 8.

## Pitfalls

- **Search starts after the header.** `ff d8 ff d9` is not a complete JPEG: the footer search begins at the byte after the three header bytes.
- **First footer wins.** A second JPEG header before the first footer does not start a new object; the first object swallows it. A carver cannot know it was a different file.
- **CRC bytes count.** `IEND` followed by only three bytes is a truncated PNG, so check `n - j >= 8` instead of computing `j + 8`.
- **Bound everything.** Check the header against the bytes that remain, check `count < max` before every store, and use subtraction rather than additions that might wrap.
- **Offsets are in the image**, not relative to the last object.

## What a defender concludes

A carved range is a candidate. Record the offset, the length and a checksum of the exact bytes so the finding can be reproduced from the untouched image. Do not describe it as "the original photo": the blocks may have been partly overwritten, fragmented or may be a coincidence of bytes. Work on a copy and keep the original image read-only.
