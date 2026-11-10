---
title: A byte-level view
summary: Produce an xxd-style display with fixed offsets, paired hex bytes and a printable ASCII column.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/hex-dump
mode: harness
---
## The forensic question

A parser says a file's header is "42 bytes long" and a tool says it contains a name. Before you trust either claim, you look at the raw bytes. A hex dump shows offsets, values and any text that happens to be in them, so an embedded zero, a stray control byte or a truncated record stands out immediately.

## The format

Each row shows up to sixteen bytes: an eight-digit hexadecimal offset, a colon and a space, the bytes as hex pairs, two spaces, then a printable view. Here are 17 bytes of synthetic evidence, `Hello, evidence!` followed by a newline:

```text
00000000: 4865 6c6c 6f2c 2065 7669 6465 6e63 6521  Hello, evidence!
00000010: 0a                                       .
```

The columns, counted from the start of a row:

```text
0-7    offset, 8 hex digits       00000010
8-9    ": "                       colon, space
10-48  hex field, 39 characters   "0a" then 37 spaces of padding
49-50  two spaces
51...  printable view, one char per byte, then "\n"
```

A full row is 10 + 39 + 2 + 16 + 1 = 68 characters; a short row has the same width up to the text column so the right-hand columns line up.

## Worked example

Take the three bytes `41 00 7f`.

1. Offset is 0, so the row starts `00000000: `.
2. Bytes are written in pairs with a space between pairs: `4100 7f`. A pair is complete after every second byte, so the separator goes before each even index after zero.
3. That text is 7 characters; pad with 32 spaces to reach 39.
4. Two spaces, then the printable view: `A` is 0x41 so it stays, `00` and `7f` are outside 0x20 to 0x7e so both become dots: `A..`.

The row is `00000000: 4100 7f` + 32 spaces + two spaces + `A..` + newline.

## Pitfalls

- **Unprintable bytes.** Only 0x20 to 0x7e are shown as themselves. Do not call `isprint`: its answer depends on the locale, and a dump of evidence must not.
- **Short last row.** Pad the hex field, or the text column drifts left and the dump misleads the reader.
- **Offsets advance by 16 per row**, not by the number of characters printed. A 4097-byte input has a last row at offset `00001000`.
- **Output size.** Build the whole text mentally first: the function returns the full length needed, like `snprintf`, even when the caller's buffer is smaller. Never write past `cap`, and always terminate when `cap > 0`.
- **Pairs.** `4865 6c6c` groups two bytes. A hex dump that uses one byte per group will be 16 columns wider and mismatch the format.

## What a defender concludes

A dump is a view, not an interpretation. If it shows `PK` followed by `03 04` you have a ZIP header to examine; if the printable column reads `.exe` inside a "photo", that is worth a note. Record the offsets you looked at so a colleague can reproduce the observation on the unmodified original.
