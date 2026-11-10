---
title: Ethernet boundaries
summary: Decode MAC addresses and EtherType, including one 802.1Q VLAN tag.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/ethernet
mode: harness
---
Ethernet is the first layer you meet in a capture of a local network, and everything after it is found by offset. Get the Ethernet header wrong and every later layer reads the wrong bytes. This lesson decodes the addresses, the EtherType and one optional 802.1Q VLAN tag.

## Layout

```text
Ethernet II                       one 802.1Q tag (EtherType 0x8100)
 0        6        12   14         0      6      12   14     16   18
 +--------+--------+----+---...    +------+------+----+-----+----+---...
 | dst    | src    |type| payload  | dst  | src  |8100| TCI |type| payload
 +--------+--------+----+---...    +------+------+----+-----+----+---...
  6 bytes  6 bytes  2 B   (at 14)                       2 B   2 B   (at 18)
```

MAC addresses are plain byte strings, copied as six bytes. The EtherType is big-endian: `p[12] << 8 | p[13]`. Values of 0x0600 and above name a protocol; the common ones are 0x0800 (IPv4), 0x0806 (ARP) and 0x86dd (IPv6).

## Worked example

```text
02 00 00 00 00 02  02 00 00 00 00 01  81 00  00 2a  08 00  45 00 ...
dst                src                 tag    TCI    inner  payload
```

Bytes 12..13 are `81 00`, so the frame carries a VLAN tag. The tag control information `00 2a` holds a priority and the VLAN id 42; this lesson does not interpret it. The real EtherType is the next two bytes, `08 00`, which is IPv4, and the payload starts at offset 18. Without the tag the same packet would have `08 00` at offset 12 and the payload at 14.

## Pitfalls

- **Length before read.** A frame of 13 bytes has no complete EtherType. A frame of exactly 14 is a valid header with an empty payload. After a 0x8100 EtherType you need 18 bytes, so a 17-byte tagged frame must fail.
- **Byte order.** Reading the EtherType as a native `uint16_t` gives 0x0008 on a little-endian machine. Assemble it from bytes.
- **No casts.** Do not overlay a packed struct on the buffer; copy with `memcpy` and shift.
- **Stacked tags.** If the inner type is itself 0x8100, return it as an ordinary type with payload offset 18 and let the caller decide.
- **Padding and FCS.** Short frames are padded to 60 bytes on the wire and the frame check sequence is usually dropped by capture tools. Neither is part of the header.

## The defender's view

The header is trivial, but frames with unexpected EtherTypes, double tags or truncated headers are exactly what filters and monitors must survive without crashing. Source MACs are claims: they can be changed by software, so a MAC alone does not identify a device. Useful Wireshark display filters are `eth.type == 0x0800` and `vlan`. The addresses in the lesson tests are locally administered, fictional values.
