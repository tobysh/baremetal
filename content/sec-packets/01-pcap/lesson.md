---
title: Read a classic pcap container
summary: Count complete pcap records while checking byte order, lengths and the supported link type.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/pcap
mode: harness
---
A capture is a container around frames. Classic pcap starts with a 24-byte global header and stores each captured frame behind a 16-byte record header. It is different from pcapng; this lesson deliberately supports only classic microsecond-resolution pcap, version 2.4, Ethernet link type 1.

The first four bytes select byte order: `d4 c3 b2 a1` means little-endian and `a1 b2 c3 d4` means big-endian. Decode subsequent integers in that order. The version is two 16-bit fields at offsets 4 and 6, snaplen is a 32-bit value at 16, and link type is at 20. Snaplen must be nonzero.

Each record has seconds, microseconds, captured length and original wire length, all 32 bits. Require microseconds below one million, captured length no larger than snaplen or original length, and enough remaining bytes for the captured frame. A shorter capture than wire length is valid truncation; walking beyond the container is not. Validate with subtraction instead of unchecked `offset + length`.

The reproducible files are in `content/sec-packets/fixtures/`; open sample.pcap in Wireshark. Equivalent display filter: `frame`. Compare Frame captured length with Frame length. There is no live capture in this course, and every address and payload is fictional.
