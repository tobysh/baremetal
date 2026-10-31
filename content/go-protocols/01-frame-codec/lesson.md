---
title: A Checksummed Frame Codec
summary: Define a wire format with a magic byte, type, length and CRC32, then encode and decode one frame from a byte buffer, telling "need more bytes" from "this is broken".
minutes: 30
difficulty: medium
tags: framing, crc32, binary
id: go-protocols/frame-codec
mode: harness
---
A byte stream has no message boundaries. A protocol must add them, and must detect damage. Every lesson in this track uses one small format:

```
+------+------+----------+-------------+-------------------+
| 0xBE | type | len (u16 | payload     | CRC32 (IEEE, u32  |
|      | u8   | big end.)| len bytes   | big endian)       |
+------+------+----------+-------------+-------------------+
 byte 0  1       2..3       4..4+len-1    last 4 bytes
```

The checksum covers every byte before it (magic, type, length and payload). The payload is at most `MaxPayload` = 1024 bytes.

## Three different failures

`Decode` must distinguish:

- `ErrShort`: the bytes so far are a valid *prefix* of a frame. A caller with a stream should wait for more bytes. This is not a fault.
- `ErrMagic`, `ErrTooLarge`, `ErrChecksum`: the bytes are wrong. Waiting will not help.

Two rules make this safe. Reject a bad magic byte as soon as you see byte 0, and reject an oversized length as soon as the 4 header bytes are present. Otherwise a peer announcing a 60 000-byte frame makes you wait for (and buffer) data you will refuse anyway.

## Limits of a checksum

CRC32 detects accidental damage such as bit flips and truncation. It is not authentication: anyone can compute a valid CRC for forged data. This track models accidents, not attackers.
