---
title: Read TCP segments
summary: Decode ports, sequence numbers and flags while respecting the TCP data offset.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/tcp
mode: harness
---
TCP presents a byte stream, but a capture contains individual segments. The first four bytes are source and destination ports, followed by 32-bit sequence and acknowledgement numbers. The upper nibble of byte 12 gives the header length in four-byte words. Byte 13 contains the ordinary FIN/SYN/RST/PSH/ACK/URG/ECE/CWR flags.

Require a complete twenty-byte base header, then validate the data offset before calculating payload length. Options may extend the header up to sixty bytes. The caller has already removed Ethernet/IP headers and restricted the span to the IP payload. Ethernet padding is therefore not counted as TCP data.

A SYN with ACK clear often starts a connection, but a single segment does not establish a complete handshake or an attack. Sequence arithmetic wraps at 2^32 and real reassembly must handle retransmissions, overlaps, loss and bidirectional streams. This lesson only decodes fields and lengths; it does not validate checksums or TCP option syntax. Equivalent filters: `tcp` and `tcp.flags.syn == 1 && tcp.flags.ack == 0`.
