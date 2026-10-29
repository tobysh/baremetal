---
title: Compare an evidence manifest
summary: Use a toy checksum manifest to distinguish matching, changed and unlisted evidence items.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/hash-verification
mode: harness
---
A manifest records a name and digest for each evidence item. Recomputing the digest can reveal a changed copy, but only if the reference manifest itself is trustworthy. In real investigations preserve the original, authenticate the reference and document custody. This exercise uses FNV-1a only because its arithmetic is small enough to inspect; FNV is not collision resistant or suitable for adversarial forensic integrity.

The 64-bit teaching checksum starts at `0xcbf29ce484222325`. For each byte, XOR it into the state and multiply by `0x100000001b3` modulo 2^64. Unsigned `uint64_t` wraparound is intentional. An empty item has the initial state as its digest.

Manifest lines contain sixteen hex digits, two spaces and a filename without spaces. Match full filenames, independently of line order. For each supplied item record status 0 if its digest matches, 1 if the name exists but the digest differs, or 2 if the name is absent. Report only supplied items; detecting missing evidence requires a separate manifest-to-inventory pass and is outside this function's contract.
