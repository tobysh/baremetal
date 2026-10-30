---
title: Build a binary payload capsule
summary: Write an invented container header with byte lengths, entry offset and a code checksum.
minutes: 40
difficulty: hard
tags: security, toy-maldev, payload-capsule
id: sec-toy-maldev/payload-capsule
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

A loader needs boundaries: where the payload starts, how much data exists, and where execution begins. An explicit container turns those assumptions into fields that can be checked. This lesson builds a capsule as bytes; the later loader validates its contents before interpreting anything.

The capsule format is invented for this course; it is distinct from the analysis course's BMX files.

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | ASCII magic `BML1` |
| 4 | 2 | entry **byte offset** into code, little-endian uint16 |
| 6 | 2 | code byte length, little-endian uint16 |
| 8 | 4 | IEEE CRC32 of code bytes, little-endian uint32 |
| 12 | length | code bytes |

Code length is 1–4096; entry must be smaller than length. No trailing bytes are allowed. CRC32 detects accidental corruption; an attacker can recompute it, so it does not establish authenticity.


Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
