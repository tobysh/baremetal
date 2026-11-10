---
title: File signatures
summary: Identify a small set of file formats from complete magic bytes at offset zero.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-forensics/file-signatures
mode: harness
---
## The forensic question

An analyst is handed a folder of files recovered from a workstation and told "these are all photos". The names say `holiday.jpg`, `invoice.pdf`, `notes.txt`. A filename is just a label somebody typed, so the first question is: what do the bytes say these files are?

Many formats begin with a fixed byte pattern, a **signature** or **magic number**. A hex dump of the first bytes of three synthetic files looks like this:

```text
file          first bytes                     reads as
holiday.jpg   ff d8 ff e0 00 10 4a 46 ...     JPEG  (ff d8 ff)
invoice.pdf   25 50 44 46 2d 31 2e 37 ...     PDF   ("%PDF-")
notes.txt     89 50 4e 47 0d 0a 1a 0a ...     PNG   (8-byte signature)
```

The third row is the finding: `notes.txt` is not text. Its first eight bytes are the PNG signature, so the extension disagrees with the content.

## The signatures you will detect

```text
name  bytes (hex)                 length
PNG   89 50 4e 47 0d 0a 1a 0a     8
JPEG  ff d8 ff                    3
GIF   47 49 46 38 37 61           6   ("GIF87a")
GIF   47 49 46 38 39 61           6   ("GIF89a")
PDF   25 50 44 46 2d              5   ("%PDF-")
ZIP   50 4b 03 04                 4   (local file header)
ELF   7f 45 4c 46                 4   (0x7f "ELF")
```

## Worked example

Take the evidence `50 4b 05 06 00 00 ...` (22 bytes). Step by step:

1. `50 4b` matches the start of the ZIP marker, but the third byte is `05`, not `03`.
2. No row of the table matches all of its bytes, so the answer is `unknown`.

That input is a real thing: an empty ZIP archive ends with a different marker (`50 4b 05 06`). A detector that only compared two bytes would call it a normal ZIP.

Now a truncated one: `89 50 4e 47 0d` with n = 5. The PNG row needs eight bytes and only five exist. Calling `memcmp(p, sig, 8)` would read three bytes past the evidence. Check the length first, then compare.

## Pitfalls

- **Bytes, not strings.** Signatures can contain zeros and nonprintable bytes, so `strcmp` and `strlen` are wrong. Use `memcmp` with an explicit count.
- **Offset zero only.** A PNG signature in the middle of a file is an embedded object, not the file's own format.
- **Length first.** `n >= signature_length` must be true before the comparison.
- **Near misses.** `GIF88a` and `%PDF` (without the dash) are not on the list.

## What a defender concludes

A matching signature is an observation: "these bytes look like a PNG". It is not proof of a valid, complete or harmless image. A truncated file can start correctly and an attacker can prepend any bytes they like. Report a mismatch between extension and signature as something to examine, and say exactly which formats your detector supports.

All evidence in this course is fictional and generated with fixed seeds. No personal data or real disk image is used.
