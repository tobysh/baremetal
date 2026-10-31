---
title: Expansion and Decompression Bombs
summary: Cap decoder output before allocating it, use a ratio limit with a floor, and guarantee stored data never grows by more than one byte.
minutes: 45
difficulty: medium
tags: limits, dos, decompression-bomb
id: go-compression/expansion-and-bombs
mode: harness
---
Two different size problems hide in every compressor.

## 1. Compression can expand

No lossless compressor shrinks every input (there are fewer short outputs than inputs). RLE on random bytes grows by about 1%. A format should therefore have a **stored** mode and a rule to pick it: `Pack` writes a mode byte, `0` = raw bytes, `1` = RLE, and uses RLE **only if strictly shorter** than raw. The worst case is then +1 byte, whatever the input.

## 2. Decompression can explode

An RLE repeat token turns 2 bytes into 129: a ratio of 64. A 2 MB file of `ff 41` tokens expands to 129 MB; gzip and zip have bombs reaching a million to one. The decompressor's output size is chosen by whoever wrote the input. Defences:

- **Absolute cap** (`MaxOutput`): the caller decides how much it is willing to hold. Check **before** you append, not after: if a token would push the total over the cap, fail without allocating or copying it.
- **Ratio cap** (`MaxRatio`): output may not exceed `MaxRatio` times the input consumed so far. This catches bombs against a big absolute cap, but tiny legitimate inputs (`ff 41` is a 64:1 ratio) would be rejected, so the ratio is enforced only once output exceeds `RatioFloor` bytes.
- Report it as its own error (`ErrTooLarge`) so callers can tell "too big" from "corrupt".

Rules, applied per token in order: (1) if the token is cut short, `ErrTruncated`; (2) let `total` be output so far plus this token, `consumed` the input bytes through this token; if `total > MaxOutput`, `ErrTooLarge`; (3) if `MaxRatio > 0` and `total > RatioFloor` and `total > MaxRatio*consumed`, `ErrTooLarge`.

Limitation: these limits bound memory and CPU for *this* decoder. They do not stop a caller that keeps asking, or a client that sends a million small valid inputs.