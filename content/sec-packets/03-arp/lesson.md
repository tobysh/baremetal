---
title: ARP observations
summary: Read an Ethernet/IPv4 ARP request or reply without treating an address claim as authenticated identity.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/arp
mode: harness
---
ARP asks which local hardware address is associated with an IPv4 address. Its fields are claims in the packet, not authentication. Duplicate or changing claims can warrant investigation, but one ARP message cannot establish spoofing or the identity of a device.

This lesson accepts Ethernet/IPv4 ARP only: hardware type 1, protocol type 0x0800, hardware length 6 and protocol length 4. A complete message is 28 bytes. Decode the operation at bytes 6..7 (1 request, 2 reply), sender IPv4 bytes 14..17 and target IPv4 bytes 24..27. All integer fields use big-endian order. The Ethernet header was removed by the caller.

Return addresses as canonical numeric values: bytes 192,0,2,10 become 0xc000020a on every host. This differs from interpreting the bytes in native machine order. Ignore trailing link padding. Equivalent Wireshark display filters are `arp` and `arp.opcode == 1`; inspect sender and target protocol addresses. All addresses in these fixtures are reserved for documentation.
