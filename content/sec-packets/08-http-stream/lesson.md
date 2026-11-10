---
title: Reassemble a tiny HTTP stream
summary: Reorder a bounded one-way TCP byte sequence and extract a GET path only after complete headers arrive.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/http-stream
mode: harness
---
Packets are not messages. TCP delivers a byte stream, and a capture shows that stream cut into segments that may arrive out of order, twice, or not at all. To read a request line you must first put the bytes back in their places. This lesson does it for one direction of one fictional connection with a known starting sequence number.

## Layout

```text
Sequence space (base = 1000, the first byte of the stream)

 seq:     1000       1007                    1024
          |          |                       |
          +----------+-----------------------+
 seg B:   |          | tes HTTP/1.1\r\n\r\n   |   arrived first
 seg A:   | GET /no  |                       |   arrived second
          +----------+-----------------------+
 offset:  0          7                       24
```

Each segment's offset in the buffer is `seq - base`. A bitmap of "byte present" values sits beside the buffer, because a stored 0 byte must not be confused with a byte that never arrived.

## Worked example

Two segments: `{seq 1007, "tes HTTP/1.1\r\n\r\n"}` arrives first and `{seq 1000, "GET /no"}` second. Offsets are 7 and 0. Written into the buffer, the bytes cover offsets 0 to 23 with no gap, so the stream is `GET /notes HTTP/1.1\r\n\r\n`. After `GET ` the path runs to the next space: `/notes`, 6 bytes. The line must end ` HTTP/1.1\r\n` and the headers must end with a blank line (`\r\n\r\n`). If a third segment repeated bytes 0 to 6 with a changed byte, the overlap would conflict, and the stream is rejected rather than guessed at.

## Pitfalls

- **Wrap and range.** Reject `seq < base` and offsets that do not fit in 1024 bytes. Use `delta > 1024 || n > 1024 - delta`, not `delta + n > 1024`, which can wrap. (Real TCP wraps at 2^32; this lesson excludes it.)
- **Gaps.** Every byte from 0 to the highest end must be present. A missing first byte means the request line is unknown, not empty.
- **Overlaps.** Identical retransmissions are normal. Different bytes at the same offset are an ambiguity, and different stacks pick different winners.
- **Completeness.** The headers must end in a blank line, otherwise the capture holds only part of the request. A packet filter cannot tell you that; following the stream can.
- **Output.** Reassemble and validate everything first. Copy the path last, with room for the terminator, so a failure leaves `out` alone.

## The defender's view

Evasion tools have long split an attack across segments, reordered them, or sent overlaps that the monitor and the target interpret differently. A defender normalises the stream the same way the endpoint does, or flags ambiguous overlaps as suspicious. This is a bounded subset: no bodies, pipelining, chunking or real reassembly. Use `tcp.port == 80` then Follow > TCP Stream in Wireshark. The host and path here are fictional.
