---
title: Walk a record stream
summary: Walk a synthetic TLS-style record stream in a growing buffer: validate each header, count complete records, and report when more bytes are needed.
minutes: 35
difficulty: medium
tags: security, protocol-analysis, parsing
id: sec-packets/record-stream
mode: harness
---
TCP delivers a stream of bytes, not messages, so a protocol that sits on top of it must find its own record boundaries. TLS records begin with a five byte header: a content type, a two byte protocol version and a two byte length, followed by that many bytes. A receiver collects bytes as they arrive and repeatedly asks the same question of the buffer: is there a complete valid record at the front?

Three answers are possible and callers must not confuse them. The buffer may end exactly on a record boundary, which is a clean state. It may end in the middle of a header or a body, which means "wait for more data" and is not an error. Or a complete header may be invalid, which means the peer is sending something this parser does not accept, and the right response is to stop reading, not to wait. A header can be judged as soon as all five bytes of it are present; it need not wait for the body.

This lesson accepts a teaching subset: content types 20 to 23, version major 3 with minor 1 to 4, and lengths from 1 to 16384. Real implementations reject other values the same way, and bound the size of buffering per connection so a peer cannot make the receiver store unbounded data. The bytes are synthetic and carry no real session.
