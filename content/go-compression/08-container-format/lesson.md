---
title: Capstone: A Container Format
summary: Combine varints, RLE, Pack, CRCs and limits into a chunked BMZ file format whose reader rejects every kind of damage; then evolve it to version 2.
minutes: 90
difficulty: hard
tags: container, format, capstone, compatibility
id: go-compression/container-format
mode: harness
---
File formats are where the earlier lessons meet. This capstone builds **BMZ version 1**:

```
"BMZ" 0x01                       magic and version
chunk*                           zero or more chunks
0x00                             end marker (uvarint 0); nothing may follow

chunk = uvarint rawLen (>= 1)    size of the chunk before compression
        uvarint encLen (>= 1)    size of the bytes below
        enc[encLen]              Pack(raw): mode byte, then data
        crc32[4]                 big-endian CRC-32 (IEEE) of the raw chunk
```

`Compress(data, chunk)` splits the input into chunks of at most `chunk` bytes. `Decompress(file, maxOutput)` must never produce more than `maxOutput` bytes.

## The reader's rules

Every size field is hostile. Check in this order for each chunk, so the error names the real problem:

1. `rawLen` varint: incomplete is `ErrTruncated`, malformed is `ErrCorrupt`. `0` is the end marker; any byte after it is `ErrCorrupt`.
2. `rawLen` larger than the output budget left: `ErrTooLarge` **before** reading further or allocating.
3. `encLen`: `0` is `ErrCorrupt`; larger than the remaining file is `ErrTruncated` (never allocate for it).
4. Unpack the chunk with `MaxOutput = rawLen`. Any error, or a result whose length is not `rawLen`, is `ErrCorrupt`: the chunk lies about its own size.
5. Fewer than 4 bytes left for the CRC: `ErrTruncated`; mismatch: `ErrChecksum`.

Header: bytes that are not a prefix of `BMZ` are `ErrFormat`; fewer than 4 bytes is `ErrTruncated`; any version other than 1 is `ErrVersion`.

Two properties follow and are tested: **every proper prefix** of a valid file is `ErrTruncated`, and **no single flipped bit** yields wrong data without an error.

## Challenge: version 2 (optional)

The lesson has a separate challenge exercise that adds version 2 while old files keep working. It cannot be passed by the base solution: it needs new functions and format rules.

Limitations: the CRC detects accidents, not tampering (anyone can recompute it). RFC-style formats add a whole-file check; here the end marker is not itself covered by a checksum.