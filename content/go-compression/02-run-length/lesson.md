---
title: Run-Length Encoding
summary: A byte-oriented RLE with literal and repeat tokens, a deterministic encoder, a strict decoder and a worst-case size bound.
minutes: 30
difficulty: easy
tags: rle, encoding, compression
id: go-compression/run-length
mode: harness
---
**Run-length encoding** replaces repeated bytes with a count. It is the simplest compressor, and it shows the two problems every compressor has: what to do with data that does *not* repeat, and what the decoder must do with a stream that lies.

## The format

A stream is a sequence of tokens. The first byte of a token is a control byte `c`:

- `c` in `0..127`: a **literal** token. `c+1` raw bytes follow (1 to 128).
- `c` in `128..255`: a **repeat** token. One byte follows; output it `c-128+2` times (2 to 129).

`aaabcc` encodes as `81 61 02 62 63 63`: a repeat of `a` three times, then a literal of 3 bytes. Note `cc` stays inside the literal, because a repeat token costs 2 bytes and would save nothing.

## One canonical encoder

Many streams decode to the same bytes. For tests (and for diffing and checksumming files) the encoder must produce **exactly one** output. The rule: at each position look at the run of equal bytes (at most 129). A run of 3 or more becomes a repeat token (flush the pending literals first); otherwise the byte joins the pending literals (flush at 128).

The decoder is looser: it must accept any valid stream, including a repeat of 2 that our encoder would never write.

## Worst case

Random data has no runs, so every 128 bytes cost 1 extra control byte. No lossless compressor can shrink all inputs; the honest guarantee is a **bound**: output is at most `len + ceil(len/128)`.