---
title: Reading a Header Safely
summary: Read little-endian fields from untrusted bytes without panicking, and validate a fixed header and the table extents it announces.
minutes: 30
difficulty: easy
tags: binary-formats, parsing, bounds-checking
id: go-executable-formats/reading-a-header
mode: harness
---
Every executable format starts with a header, and every loader starts by reading it. The bytes come from a file you did not write, so the first skill is reading numbers out of them **without trusting them**.

## Outcome

You can read fixed-width fields with explicit bounds checks and reject a header whose claims do not fit the file. After this lesson a short, damaged or hostile file produces an error value, never a panic.

## Little-endian fields

`encoding/binary` turns four bytes into a number: `binary.LittleEndian.Uint32(b[off:])`. It panics when fewer than four bytes remain. The check belongs in your code:

```go
if off > uint64(len(b)) || uint64(len(b))-off < 4 { /* too short */ }
```

Notice the shape. `off+4 > len(b)` looks the same but wraps around when `off` is near the top of the integer range, and then the check passes when it should fail. Subtract on the side that cannot underflow.

## The object header

Our toy object format (TOBJ) starts with 16 bytes: magic `TOBJ`, a version, a flags byte that must be zero, three 16-bit counts (sections, symbols, relocations) and the size of the string table. The tables follow the header back to back: sections, symbols, relocations, strings. The header therefore tells you where each table starts and ends, and a **lying count** can place the end far beyond the file. Compute the extents in 64-bit arithmetic and compare them with the real length before anyone reads a table.

Errors wrap sentinel values (`ErrTruncated`, `ErrMagic`, `ErrVersion`, `ErrCorrupt`) with `%w`, so callers can write `errors.Is(err, ErrTruncated)` and tests can tell *why* a file was refused.

The format is **invented for this course** (named TOBJ for objects and TEXE for executables). It borrows ideas from ELF and a.out but is far smaller, so every byte can be accounted for. It shows how linkers work, not how any real platform loads programs. Real formats add alignment rules, dynamic linking, position independence and much more.
