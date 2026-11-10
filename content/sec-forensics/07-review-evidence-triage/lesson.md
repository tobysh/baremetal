---
title: Review: evidence triage
summary: Combine signatures, a short hex preview and a toy checksum into a reproducible evidence observation.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/review-evidence-triage
mode: harness
kind: review
---
This review combines [signatures](/learn/sec-forensics/file-signatures), [hex display](/learn/sec-forensics/hex-dump) and [manifest checks](/learn/sec-forensics/hash-verification). No new file format appears. The goal is the report an analyst would paste into case notes: one reproducible line per item that states observations and nothing more.

## The question

For each item you are given a name, the bytes, and the digest recorded in the acquisition manifest. Triage asks three separate things: what do the first bytes look like, what are they exactly, and do they still hash to the recorded value?

```text
name: TYPE, first bytes HEX, hash DIGEST, STATUS
```

## Worked example

```text
item1.bin   50 4b 03 04 67 09 fd e8 ... (15 bytes)   manifest 56b42e46a0733002
```

1. Signature: `50 4b 03 04` is the ZIP local header, so `ZIP`.
2. Preview: the first eight bytes in lowercase, single spaces: `50 4b 03 04 67 09 fd e8`.
3. Digest: compute FNV-1a over all 15 bytes and print it as sixteen lowercase hex digits, padded with zeros.
4. Status: `OK` if the computed digest equals the manifest value, `CHANGED` otherwise.

```text
item1.bin: ZIP, first bytes 50 4b 03 04 67 09 fd e8, hash 56b42e46a0733002, OK
```

Note that the printed hash is always the one you computed, even for `CHANGED`, so a colleague can see both what you found and compare it with the manifest.

## Limits of this report

- Only PNG, JPEG, PDF and ZIP are recognised here. A GIF or an ELF file is reported as `unknown`, which means "not one of the formats this report covers", not "unrecognisable".
- A short item shows only the bytes it has; an empty item shows `-` for the preview and the initial FNV state as its digest.
- A truncated signature (a 7-byte PNG start) is `unknown`.
- `OK` means "equal to the reference under a noncryptographic checksum". It never means authentic.

## Pitfalls

- A preview of exactly eight bytes must not end with a trailing space, and nine bytes still show eight.
- `%016llx` needs `unsigned long long`; casting `uint64_t` avoids a mismatched format on some platforms.
- The output is bounded: use `snprintf` with the caller's capacity and terminate it when `cap > 0`. With `cap == 0` write nothing.

## What a defender concludes

Keep observations separate from conclusions. "Type ZIP by signature, digest differs from the manifest" is an observation; "the attacker modified the archive" is a hypothesis that needs more evidence, such as timestamps, the manifest's own provenance and an inspection of what actually changed.
