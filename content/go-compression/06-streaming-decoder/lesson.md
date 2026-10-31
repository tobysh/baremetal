---
title: A Streaming Decoder
summary: Decode a Huffman stream fed in chunks of any size, emitting symbols as soon as their bits arrive, with sticky errors.
minutes: 60
difficulty: hard
tags: streaming, state-machine, huffman
id: go-compression/streaming-decoder
mode: harness
---
Real data arrives in pieces: network reads, 4 KiB file blocks, a single byte. A decoder that needs the whole input in memory cannot start until the last byte arrives and cannot bound its memory. A **streaming decoder** keeps *state* between calls.

```go
d := NewStreamDecoder()
for each chunk { out, err := d.Write(chunk); ... }
err := d.Close()
```

## What state?

Everything the loop of `HuffDecode` kept in local variables must now live in the struct:

- the header bytes seen so far (at most a few hundred: the header is bounded, so buffering it is safe; buffering the *payload* is not allowed);
- which phase you are in: header, payload, or done;
- the code bits accumulated for the symbol in progress (`code`, `clen`), which can be half-read at a chunk boundary;
- how many symbols are decoded so far, against `n`.

## Rules that make it testable

- **Chunk invariance.** However the bytes are split (one at a time, empty writes, any sizes), the concatenated output is the same as `HuffDecode`'s, and the same inputs are accepted or rejected.
- **Promptness.** After each byte, every symbol whose last bit has arrived must already be returned. The tests count output after every single byte.
- **Sticky errors.** After an error every call returns the same error and no output. The caller must not be able to continue past a corrupt stream.
- **Close is a verdict.** `Close()` returns nil only if exactly `n` symbols were decoded; `ErrTruncated` if the stream ended early.
- **Trailing data.** A byte arriving after the stream is complete is `ErrCorrupt`, immediately, in `Write`.

Limitation: output for a stream that later turns out to be corrupt (for example bad padding) has already been returned. A streaming decoder cannot take it back; consumers that need all-or-nothing must buffer, or verify a checksum at the end (the container does).