---
title: "Review: Framed Messages"
summary: A consolidation exercise. Combine byte order, full reads and writes and a length prefix to send discrete messages over a byte stream.
minutes: 25
difficulty: medium
kind: review
tags: framing, sockets, tcp
id: c-networking/review-framed-messages
mode: harness
---
This is a **review lesson**. Nothing here is new. You have met all the ingredients:

- **Byte order** ([Byte Order](/learn/c-networking/byte-order)): multi-byte numbers on the wire are big-endian.
- **Full reads and writes** ([Partial Reads and Writes](/learn/c-networking/partial-io)): a single `read` may return part of what you asked for.
- **Framing** ([Framing a Line Protocol](/learn/c-networking/line-framing)): a stream needs a rule that says where each message ends.
- **Sockets** ([A TCP Echo Server](/learn/c-networking/tcp-echo-server)): descriptors that you read and write.

## Length-prefixed frames

Lines are fine for text. For binary data, which may contain any byte including `\n`, the usual framing is a **length prefix**: a 2-byte big-endian length, then exactly that many bytes of payload.

```
00 05 h e l l o   00 00   00 03 a b c
 len  payload      empty   len payload
```

The reader first does a full read of 2 bytes, then a full read of `len` bytes. No searching, no scanning, and a frame can hold anything.

## The small twist

The echo server in this exercise does not echo; it replies with each payload **reversed**, and it stops when the client closes the connection between frames. That makes it obvious from the tests that you really delimited the frames correctly.

`read_full`, `write_full`, `put_u16` and `get_u16` are in the starter, so just assemble them.

## Defensive details

- A frame longer than the sender may use (1024 bytes here) is refused rather than allocated.
- A header promising 500 bytes into a buffer that holds 100 must not overflow it.
- "The peer closed before the next frame" (a clean end) is different from "the peer closed halfway through a frame" (a truncated message).
