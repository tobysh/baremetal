---
title: ARP observations
summary: Read an Ethernet/IPv4 ARP request or reply without treating an address claim as authenticated identity.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/arp
mode: harness
---
ARP asks which hardware address currently belongs to an IPv4 address on the local link. Every field is a claim made by whoever sent the packet. Nothing in ARP authenticates the sender, so one message can never prove spoofing or identify a device; what it gives you is an observation to compare with other observations.

## Layout

```text
ARP for Ethernet/IPv4 (28 bytes, Ethernet header already removed)
 0     2     4    5    6     8          14        18          24       28
 +-----+-----+----+----+-----+----------+---------+-----------+--------+
 |htype|ptype|hlen|plen| op  | sender   | sender  | target    | target |
 | 0001| 0800| 06 | 04 |     | MAC (6)  | IP (4)  | MAC (6)   | IP (4) |
 +-----+-----+----+----+-----+----------+---------+-----------+--------+
```

All multi-byte integers are big-endian. The operation is 1 for a request ("who has this IP?") and 2 for a reply ("I have it"). This subset rejects anything else.

## Worked example

```text
00 01  08 00  06  04  00 01  02 00 00 00 00 01  c0 00 02 0a  00 00 00 00 00 00  c0 00 02 01
htype  ptype  hl  pl  op=1   sender MAC          sender IP     target MAC (0)     target IP
```

Reading it: Ethernet hardware, IPv4 protocol, 6-byte and 4-byte addresses, operation 1. The sender claims MAC `02:00:00:00:00:01` owns `c0 00 02 0a`, which is 192.0.2.10, and asks who has `c0 00 02 01` (192.0.2.1). The target MAC is zeroed because a request does not know it yet. As a number, 192.0.2.10 is `0xc000020a` on every machine, because it is built from the bytes in order: `(uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]`. Casting the pointer to `uint32_t *` would give `0x0a0200c0` on a little-endian host and can also fault on unaligned data.

## Pitfalls

- **Check the format fields first.** The fixed offsets 14 and 24 only mean "sender IP" and "target IP" when htype is 1, ptype is 0x0800, hlen is 6 and plen is 4. A packet with plen 16 has a different layout.
- **Shifting a byte.** `p[0] << 24` promotes to a signed `int`, which overflows for values of 128 and up. Cast to `uint32_t` first. The tests use addresses that start with 128 or more for this reason.
- **Length.** Require at least 28 bytes; accept more, because Ethernet pads short frames to 60 bytes.
- **Operation.** Operations 0, 3 and above belong to other protocols such as RARP and are rejected here.

## The defender's view

A reply nobody asked for, two replies disagreeing about one IP, or a MAC that suddenly changes are the patterns defenders compare across a capture. Tools that watch for them record the claims and the time, and then investigate on the network; a single packet is never a verdict. In Wireshark use `arp` and `arp.opcode == 1`. Every address here is in a documentation range.
