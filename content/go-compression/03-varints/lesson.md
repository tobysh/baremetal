---
title: Varints and Zigzag
summary: Encode integers in 7-bit groups, reject truncated, overlong and overflowing input, and map signed numbers with zigzag.
minutes: 35
difficulty: medium
tags: varint, encoding, validation
id: go-compression/varints
mode: harness
---
Lengths and counts are usually small, but a fixed 8-byte field wastes space. A **varint** stores an unsigned integer in 7-bit groups, least significant group first. The top bit of each byte says "more follows".

```
300 = 0b10_0101100  ->  1_0101100  0_0000010  =  ac 02
```

Protocol Buffers, DWARF, WebAssembly and Go's `encoding/binary` all use this (LEB128).

## A decoder meets hostile bytes

A decoder must make three decisions, each with its own error:

- **Truncated**: the input ends while the continuation bit is still set. `ErrShort`. A streaming caller can wait for more data.
- **Overflow**: the value does not fit in 64 bits. A 64-bit value needs at most 10 bytes, and the 10th may only be `0x00` or `0x01`. Anything else is `ErrOverflow`.
- **Non-canonical**: `80 00` also means 0, with a wasted byte. Accepting several encodings of one number breaks signatures, caches and diffing, and has been used to smuggle values past filters. Reject a final byte of `0x00` unless it is the only byte: `ErrNonCanonical`.

## Signed numbers: zigzag

`-1` as a `uint64` is 18446744073709551615, ten bytes. **Zigzag** maps small magnitudes to small codes: `0, -1, 1, -2, 2` become `0, 1, 2, 3, 4`. The formula is `(v << 1) ^ (v >> 63)`, with an arithmetic shift on the signed value.