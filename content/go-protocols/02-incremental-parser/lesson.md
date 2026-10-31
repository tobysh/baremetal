---
title: An Incremental Parser
summary: Turn arbitrary chunks of a byte stream into frames. The answer must not depend on where the chunk boundaries fall.
minutes: 35
difficulty: medium
tags: parsing, streams, buffering
id: go-protocols/incremental-parser
mode: harness
---
A TCP-like stream delivers bytes in pieces of any size. One `Read` may return half a header, three frames and the start of a fourth, or a single byte. A parser that works when you hand it a whole message and breaks when you hand it the same bytes in two pieces is the classic framing bug.

The rule to test is a **property**: for a given byte stream, the sequence of frames produced must be identical for *every* way of cutting it into chunks.

## Design

Keep a buffer of bytes not yet consumed. `Feed(chunk)` appends to it, then repeatedly asks `Decode` for a frame at the front:

- success: emit the frame, drop its `n` bytes, continue;
- `ErrShort`: stop and wait for the next chunk;
- any other error: the stream is damaged. This lesson is strict: the parser **latches** the error and refuses all further input. (Lesson 6 recovers instead.)

Two memory rules: do not keep a reference to the caller's chunk (callers reuse buffers), and do not hand out frames whose payload aliases your internal buffer. `Decode` already copies payloads.

The buffer is bounded: after a `Feed` it can never hold a whole frame, so it stays under `HeaderLen+MaxPayload+TrailerLen` bytes. The codec from lesson 1 is provided.
