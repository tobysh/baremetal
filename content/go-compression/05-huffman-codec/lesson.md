---
title: A Huffman Codec and Bad Input
summary: Decode a self-describing Huffman stream and reject every malformed header, bad code set, padding and truncation precisely.
minutes: 60
difficulty: hard
tags: huffman, validation, malformed-input
id: go-compression/huffman-codec
mode: harness
---
Now the pieces become a codec. The starter gives you a working **encoder** (`HuffEncode`, built from your earlier pieces) and the format; you write the part that faces untrusted bytes.

## The format

```
uvarint n            number of symbols that follow
uvarint k            number of distinct symbols used (0..256)
k x (symbol, length) byte pairs, symbols strictly ascending
bits                 canonical codes, MSB first, zero-padded to a byte
```

## A decoder's checklist

The decoder is the security boundary, so write down what "valid" means *before* coding. For this format:

- the header must be complete (`ErrTruncated`) and its varints canonical;
- `k <= 256`, symbols strictly ascending, each length in `1..48`;
- the lengths must form a **complete** prefix code (Kraft sum exactly 1), except that `k == 1` must be a single 1-bit code and `k == 0` requires `n == 0`;
- the payload must hold all `n` symbols (`ErrTruncated`), every bit pattern must be a code, the padding bits must be zero, and there must be **no bytes after** the last one needed (`ErrCorrupt`).

Two errors, two meanings: `ErrTruncated` = "more input might fix this" (streaming code in the next lesson relies on it); `ErrCorrupt` = "this can never be valid".

## Do not trust `n`

`n` is a number from an attacker. Allocating `make([]byte, n)` for `n = 2^40` kills the process. But every symbol costs at least one bit, so `n` can never exceed `8 * len(payload)`: check that first.

Limitation: this layer has **no checksum**. A flipped bit in the payload usually decodes to wrong bytes without any error. The container in the last lesson adds CRCs.