---
title: UDP datagram bounds
summary: Use the UDP length field to separate its payload from trailing captured bytes.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/udp
mode: harness
---
UDP has an eight-byte header: source port, destination port, length and checksum, each two bytes in network order. The length includes the header itself, so a value below eight is invalid. A length beyond the supplied transport span indicates a truncated or inconsistent datagram.

Do not assume every captured byte belongs to UDP. After validating `8 <= length <= n`, report `length - 8` as the payload. Trailing bytes are ignored in this bounded teaching interface. The caller supplies the transport payload, not an Ethernet frame, and has already handled IP fragmentation.

UDP has no TCP-style sequence space, acknowledgement or built-in stream ordering. A DNS-looking destination port is only a clue about an application protocol; application bytes must still be parsed. This lesson does not validate checksums: doing so requires the IP pseudo-header and differs between IPv4 and IPv6. Equivalent Wireshark filters are `udp` and `udp.port == 53`. The next lesson reads the DNS message separately so each layer has a clear contract.
