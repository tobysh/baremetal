---
title: Bit Reader and Writer
summary: Pack values of any width from 0 to 64 bits into bytes and read them back, MSB first, and detect running out of bits.
minutes: 25
difficulty: easy
tags: bits, encoding, io
id: go-compression/bit-io
mode: harness
---
Compression formats do not respect byte boundaries. A Huffman code might be 3 bits, the next 11. To write them you need a **bit writer**, and to read them back a **bit reader**.

## The bit order

This track packs **most significant bit first**: the first bit written becomes the `0x80` bit of the first byte. Writing `101`, then `1`, then `11111111`, then `00` gives the bits `101 1 11111111 00`, grouped into bytes as `1011 1111 | 1111 00` plus two padding zeros: `bf fc`. (DEFLATE uses the opposite order, least significant bit first. Both are fine as long as the writer and reader agree; always check which one a format specifies.)

```go
var w BitWriter
w.WriteBits(0b101, 3)   // the low 3 bits of the value, highest of them first
w.WriteBits(0xFF, 8)
w.Bytes()                // padded with zero bits at the end
```

## Failure is a value

Reading past the end is not a crash. `ReadBits` returns `ErrShort` and, importantly, **consumes nothing**, so a caller can wait for more input and retry. Every later lesson relies on that.

Your public example is below the starter: run it before you start and again when you finish.

Limitations: one bit at a time is slow. Real codecs keep a 64-bit buffer and refill it by whole bytes. This is the clearest version, not the fastest.