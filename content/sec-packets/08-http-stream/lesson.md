---
title: Reassemble a tiny HTTP stream
summary: Reorder a bounded one-way TCP byte sequence and extract a GET path only after complete headers arrive.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/http-stream
mode: harness
---
Packet boundaries are not application-message boundaries. A request line can be split between segments, and the capture may show segments out of order or more than once. This exercise receives already-decoded segments from one direction of one fictional connection and a known initial sequence number.

Use a 1024-byte temporary buffer plus a presence bitmap. Convert each sequence number to an offset from base, rejecting values before base or ranges outside the buffer. Identical retransmitted bytes are harmless; conflicting overlaps are rejected rather than silently choosing one interpretation. Require every byte from zero through the greatest observed end to be present. Sequence-number wrapping is deliberately unsupported here.

After reassembly, accept a small ASCII HTTP/1.1 subset: `GET `, a nonempty path without spaces, ` HTTP/1.1` and CRLF, followed by complete headers ending CRLF CRLF. Return the path only. This is not a general HTTP parser: bodies, pipelining, chunking and real TCP reassembly remain outside its contract. Equivalent Wireshark view: filter `tcp.port == 80`, then Follow > TCP Stream. A packet filter alone cannot guarantee a complete request.
