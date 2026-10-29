---
title: Packed Binary Headers
summary: Encode and decode a fixed 10 byte header with packed bit fields, and learn why you never send a struct by copying its memory.
minutes: 20
difficulty: medium
tags: binary, parsing, bit-fields
id: c-networking/packed-headers
mode: harness
---
Almost every protocol starts a message with a **header**: a fixed layout of fields at fixed byte offsets. The header in this course (we'll call it BMP) is 10 bytes:

```
offset  size  field
0       2     magic    0xB4DE
2       1     version (high 4 bits) | flags (low 4 bits)
3       1     type
4       2     length   of the payload that follows
6       4     seq      sequence number
```

Two fields share byte 2. Packing a nibble pair is done with shifts and masks:

```c
out[2] = (uint8_t)((version & 0x0F) << 4 | (flags & 0x0F));
version = buf[2] >> 4;
flags   = buf[2] & 0x0F;
```

## Why not `memcpy(out, &header, sizeof header)`?

Because the compiler is free to **pad** a struct so fields line up for the CPU. A struct holding a `uint8_t` and then a `uint32_t` is 8 bytes, not 5. The in-memory layout is also in your CPU's byte order, and `uint16_t length` would be little-endian on x86. A wire format must be defined byte by byte, so you convert field by field. (`__attribute__((packed))` removes padding but not the endianness problem, and unaligned access to packed members is a portability trap.)

## Decoding untrusted bytes

Bytes from a network may be truncated or hostile. A decoder must check *before* it reads:

1. Is there enough input? If fewer than 10 bytes have arrived, the answer is "not yet", which is different from "bad".
2. Is the magic number right? If not, reject.

Never read `buf[9]` before you know `n >= 10`.

The big-endian helpers from [Byte Order](/learn/c-networking/byte-order) are included in the starter.
