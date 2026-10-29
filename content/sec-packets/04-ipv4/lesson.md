---
title: IPv4 length claims
summary: Separate captured bytes, the variable IP header and the declared datagram length before reading transport data.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/ipv4
mode: harness
---
IPv4 carries its own length claims inside a capture. The first byte contains a version nibble and IHL nibble. Require version 4; multiply IHL by four to obtain a header length of at least twenty bytes. Options increase that length, so transport data does not always start at byte twenty.

The total length at bytes 2..3 includes the header and payload. It must be at least the header length and no larger than the supplied byte span. Extra bytes after total length may be Ethernet padding and are not transport data. Protocol is byte 9; source and destination addresses begin at 12 and 16.

The flags/fragment offset field at bytes 6..7 matters before interpreting transport headers. A nonzero offset or More Fragments bit means this datagram participates in fragmentation. The Don't Fragment bit alone does not. This function reports fragmentation but does not reassemble fragments, validate checksum/options, or implement IPv6. Equivalent Wireshark filters: `ip` and `ip.flags.mf == 1 || ip.frag_offset > 0`. The source address remains a packet claim.
