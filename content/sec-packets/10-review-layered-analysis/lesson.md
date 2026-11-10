---
title: Review: layered analysis
summary: Combine established layer bounds before classifying a frame and distinguish a port hint from a parsed DNS message.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/review-layered-analysis
mode: harness
kind: review
---
This review combines [Ethernet](/learn/sec-packets/ethernet), [IPv4](/learn/sec-packets/ipv4), [TCP](/learn/sec-packets/tcp) and [UDP](/learn/sec-packets/udp). No new protocol appears. The skill is handing the right bytes to each layer: every parser trusts the span it is given, so the caller is where mistakes happen.

## Layout

```text
frame (n bytes)
+------------------+---------------------------+----------------------+--------+
| Ethernet 14 or 18| IPv4 header, ihl*4 bytes  | transport, total-ihl*4| padding|
+------------------+---------------------------+----------------------+--------+
                    |<-------------- IPv4 total length -------------->|
 offset: eth.payload  eth.payload + ip.header

 decision:  invalid | arp | other | ipv4/fragment | ipv4/tcp |
            ipv4/udp | ipv4/udp/dns-port | ipv4/other
```

Each helper returns the offset or length that the next one needs: `Ethernet.payload`, then `IPv4.header` and `IPv4.total`. The transport span is `total - header` bytes starting at `payload + header`, not "the rest of the frame".

## Worked example

```text
frame bytes (n = 62): 14 Ethernet + 20 IPv4 + 12 UDP + 16 padding
Ethernet type 08 00  -> IPv4
IPv4  45 .. 00 20 .. 11  -> header 20, total 32, protocol 17
transport span = 32 - 20 = 12 bytes
UDP   00 35 c3 50 00 0c 00 00 ...  -> src 53, dst 50000, length 12
```

The frame is 62 bytes, but the IPv4 total length says only 32 belong to IP, so the UDP parser sees 12 bytes and finds a valid datagram of length 12. One source port is 53, so the label is `ipv4/udp/dns-port`. If you passed the UDP parser the 28 bytes left in the frame, a declared length of up to 28 would look acceptable even though 16 of those bytes are padding.

## Pitfalls

- **Order.** Decide the Ethernet type first. ARP and unknown types stop there; do not parse IP for them.
- **Fragments before transport.** A later fragment has no TCP or UDP header. Return `ipv4/fragment` before decoding it.
- **Options.** With IHL 6 the transport header starts at 24 bytes into the IP packet, so use `ip.header`, not a constant 20.
- **Padding.** Pass `ip.total - ip.header` bytes to the transport parser, so padding cannot satisfy a length check.
- **Port 53 is a hint.** The label says dns-port, never "DNS". Parsing the message would need the DNS lesson's decoder.

## The defender's view

A classifier like this is the first stage of many monitors, and the mistakes above are the ones that let malformed traffic through or crash the tool. State its limits when you report: a label describes a structure, not a verdict. The matching Wireshark filter is `arp || ip`; compare each label with the protocol tree.
