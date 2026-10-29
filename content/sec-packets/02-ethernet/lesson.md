---
title: Ethernet boundaries
summary: Decode MAC addresses and EtherType, including one 802.1Q VLAN tag.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/ethernet
mode: harness
---
An Ethernet II header starts with six destination MAC bytes, six source MAC bytes and a two-byte EtherType in network (big-endian) order. The ordinary payload begins at byte 14. MAC addresses are byte sequences; do not cast them to machine integers or unaligned structs.

When the EtherType is 0x8100, this subset accepts one 802.1Q tag: bytes 14..15 are the tag control information and bytes 16..17 are the encapsulated EtherType. The payload then starts at 18. Validate the longer header before reading it. Other types, including a second tag marker, are returned as uninterpreted types; stacked VLAN parsing is outside this exercise.

The parser reports boundaries, not whether the payload protocol is valid. Capture files normally omit the Ethernet frame check sequence and may retain padding; neither belongs in the fixed header calculation. Equivalent Wireshark display filters are `eth.type == 0x0800` for IPv4 and `vlan` for tagged frames. The lesson harness includes tags even though the small demonstration capture uses ordinary Ethernet.
