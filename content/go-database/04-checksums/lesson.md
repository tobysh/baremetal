---
title: Checksums and Corruption
summary: Frame each record with a CRC-32 so that flipped bits are detected, and tell a torn tail apart from damage.
minutes: 35
difficulty: medium
tags: crc, integrity, log
id: go-database/checksums
mode: harness
---
Last lesson a record was valid if it was *long enough*. That catches a torn write, but not a **flipped bit**: a failing disk, a bad cable or a buggy driver can change a byte and leave the length perfectly plausible. You would replay a wrong value without noticing.

A **checksum** is a small number computed from the data. Store it with the data; when you read, compute it again. If the two differ, something changed. Databases use CRC-32 for this: it is fast and is very good at catching accidental damage (it is not a defence against an attacker).

```go
sum := crc32.ChecksumIEEE(payload)
```

## A frame

Wrap each payload in a **frame**:

```
+----------+----------+-----------+
| crc u32  | len u32  |  payload  |
+----------+----------+-----------+
```

Both integers are big-endian. The CRC covers the four `len` bytes **and** the payload, so a damaged length is caught too. You can feed a CRC in pieces:

```go
c := crc32.Update(0, crc32.IEEETable, lenBytes)
c = crc32.Update(c, crc32.IEEETable, payload)
```

## Two kinds of bad ending

When you scan a log of frames there are two different reasons to stop:

- **Short**: the bytes run out before the frame is complete. That is the torn tail from a crash. It is normal, and you quietly keep what came before.
- **Corrupt**: the whole frame is there but the checksum disagrees. Data was damaged. You still stop (you cannot trust anything after it), but you report it, so a human can find out.

A caller needs to know both how far the good data goes and whether it ended cleanly.
