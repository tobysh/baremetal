---
title: A byte-level view
summary: Produce an xxd-style display with fixed offsets, paired hex bytes and a printable ASCII column.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/hex-dump
mode: harness
---
Hex dumps let an analyst check what a parser claims against the underlying bytes. Each row begins with an eight-digit hexadecimal byte offset and displays at most sixteen bytes. Pair the hex bytes in groups of two, separated by one space. Pad that hex column to 39 characters, then add two spaces and a printable view.

Only bytes 0x20 through 0x7e are printable here; replace every other value with a dot. This deliberately uses an ASCII rule rather than the process locale. A final short row still aligns its ASCII column with the full rows. Empty evidence produces an empty string.

Build a row in a temporary buffer, measuring the actual number of characters, then copy only what fits into the caller's output. The return value is the full required output length, like `snprintf`, so a caller can detect truncation. Preserve room for the terminator whenever cap is positive. This display format is a teaching tool, not a file parser; it makes offsets, embedded zeros and truncation visible without interpreting the evidence.
