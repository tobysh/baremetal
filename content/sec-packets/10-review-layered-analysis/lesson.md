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
This review combines [Ethernet](/learn/sec-packets/ethernet), [IPv4](/learn/sec-packets/ipv4), [TCP](/learn/sec-packets/tcp) and [UDP](/learn/sec-packets/udp). The supplied helpers are the same bounded field parsers; use their outputs to decide exactly which bytes belong to the next layer.

Start with Ethernet and return other for unsupported types. For IPv4, constrain the transport span to total length minus the actual header length, ignoring padding. Return a fragment classification before trying to decode a transport header. A later fragment may contain no TCP/UDP header at all.

Use the transport parser before labeling TCP or UDP. Port 53 is only a DNS-port hint, so the label deliberately says dns-port rather than asserting a valid DNS message. The earlier DNS parser would be needed to inspect the application bytes. No new protocol is introduced. Equivalent filter: `arp || ip`; compare each classification with the protocol tree in Wireshark. The goal is a reproducible description with explicit limits, not a judgment about whether traffic is safe.
