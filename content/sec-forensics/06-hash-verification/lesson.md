---
title: Compare an evidence manifest
summary: Use a toy checksum manifest to distinguish matching, changed and unlisted evidence items.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/hash-verification
mode: harness
---
## The forensic question

You acquired three files from a laptop yesterday and wrote their checksums into a manifest. Today a colleague hands you copies. Are they the same bytes? Recomputing each checksum and comparing it with the manifest answers that, as long as the manifest itself is trustworthy and was stored apart from the evidence.

## The checksum and the manifest

The teaching checksum is 64-bit FNV-1a. Start with `h = 0xcbf29ce484222325`; for every byte, `h ^= byte; h *= 0x100000001b3` (modulo 2^64, which `uint64_t` gives you). An empty file therefore has digest `cbf29ce484222325`. FNV is tiny and easy to inspect, but it is not collision resistant: do not use it against an adversary.

A manifest line is sixteen hex digits, two spaces and a filename:

```text
af63dc4c8601ec8c  a.bin
85944171f73967e8  foobar.log
cbf29ce484222325  empty.dat
```

Those digests are real: `a.bin` holds the single byte `a`, `foobar.log` holds the text `foobar`, and `empty.dat` is empty.

## Worked example

You are given four items and the manifest above:

```text
supplied item   bytes        manifest says         result
a.bin           "a"          af63dc4c8601ec8c      recompute: af63dc4c8601ec8c  -> 0 match
foobar.log      "foobaR"     85944171f73967e8      recompute differs            -> 1 changed
notes.txt       "hello"      (no such line)                                     -> 2 unlisted
empty.dat       (0 bytes)    cbf29ce484222325      recompute: cbf29ce484222325  -> 0 match
```

For each supplied item:

1. Find the manifest line whose filename is exactly the item's name. No line: status 2.
2. Otherwise recompute the digest from the bytes and compare it to the digits on that line: equal is 0, different is 1.

The function returns the count of nonzero statuses (here 2).

## Pitfalls

- **Exact names.** `a.bin` is not `a.bin.bak`. Compare whole filenames, not prefixes or substrings, and remember names are case-sensitive.
- **Order does not matter.** Search the manifest for each item rather than walking both lists in step.
- **The last line may have no newline.** Do not require one to match it.
- **Wraparound is intentional.** Keep the state in `uint64_t`; a signed type makes the multiplication undefined behaviour.
- **Only supplied items are reported.** A manifest line with no matching item is "missing evidence", a separate inventory question outside this function.
- **Parse digits as hex**, 64-bit: use `unsigned long long` with `%llx`.

## What a defender concludes

"Match" means these bytes produce the digest recorded earlier. It supports "unchanged since the manifest was written" only if the manifest was stored somewhere an attacker could not rewrite, and only for a strong hash. With FNV an attacker can craft a different file with the same digest, so in real work use SHA-256 or better, and sign or separately store the manifest.
