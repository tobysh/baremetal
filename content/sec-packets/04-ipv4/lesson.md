---
title: IPv4 length claims
summary: Separate captured bytes, the variable IP header and the declared datagram length before reading transport data.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/ipv4
mode: harness
---
IPv4 packs several length claims into one header: how long the header is, how long the whole datagram is, and how many bytes you were actually handed. A parser must believe none of them until it has checked them against each other, because the transport header you want to read next starts at an offset that comes from the packet itself.

## Layout

```text
 0       1       2       4       6        8     9      10      12       16       20
 +-------+-------+-------+-------+--------+-----+------+-------+--------+--------+
 |ver|ihl| TOS   | total | ident | flags/ | TTL | proto| cksum | src    | dst    |
 |4b |4b |       | length|       | frag   |     |      |       |        |        |
 +-------+-------+-------+-------+--------+-----+------+-------+--------+--------+
 options (if ihl > 5) follow at byte 20, then the payload at ihl*4

 flags/frag, 16 bits:  [reserved 0x8000][DF 0x4000][MF 0x2000][fragment offset 0x1fff]
```

The IHL nibble counts four-byte words, so a header is `ihl * 4` bytes, at least 20 and at most 60. Total length counts the header plus the payload. Protocol 6 is TCP, 17 is UDP and 1 is ICMP.

## Worked example

```text
45 00 00 18  00 7b 00 00  40 06 8e 13  c0 00 02 0a  c6 33 64 14  64 61 74 61
```

`45` is version 4 and IHL 5, so the header is 20 bytes. `00 18` is total length 24, matching the four payload bytes `64 61 74 61` after the 20-byte header. The flags/fragment word `00 00` has no More Fragments bit and offset 0, so this is a whole datagram. Protocol `06` is TCP, source `c0 00 02 0a` is 192.0.2.10 and destination `c6 33 64 14` is 198.51.100.20. Change `45` to `4f` and the header would be 60 bytes, so the transport data would begin at 60, not 20, and this 24-byte packet would be invalid.

## Pitfalls

- **Three lengths.** Require `ihl*4 >= 20`, `ihl*4 <= n`, `total >= ihl*4` and `total <= n`. Each one blocks a different bad read.
- **Padding.** Ethernet pads short frames. Bytes beyond `total` are not part of the datagram, so a caller must use `total - header`, not `n - header`.
- **Fragments.** Any nonzero fragment offset or the MF bit means this packet is one piece. A later fragment has no transport header at all. DF alone does not fragment, and the reserved bit is ignored: the mask is `0x3fff`.
- **Byte order.** Total length, flags and addresses are big-endian.

## The defender's view

Mismatched length fields are a classic way to hide data from a naive tool or to trigger an out-of-bounds read in one. IP options, small fragments and overlapping fragments have also been used to slip past filters that reassemble differently from the receiving host. Source addresses are claims, since they are trivially forged. This lesson reports fragmentation but does not reassemble or check the header checksum. Wireshark filters: `ip` and `ip.flags.mf == 1 || ip.frag_offset > 0`.
