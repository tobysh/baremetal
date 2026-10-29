---
title: Evaluate firewall rules
summary: Match packets against ordered allow/deny rules with CIDR networks and port ranges; the first match wins and the default is deny.
minutes: 40
difficulty: medium
tags: security, detection, network, firewall
id: sec-detection/firewall-rules
mode: stdout
---
A packet filter is an ordered list of rules. For each packet it walks the list from the top, applies the action of the first rule whose conditions all match, and falls back to a default. Defensive policy defaults to deny: anything not explicitly allowed is dropped, so a forgotten case fails closed.

A rule matches on protocol (`tcp`, `udp` or `any`), source and destination network, and destination port. Networks are written as CIDR, an address plus the number of leading bits that must match: `10.1.0.0/16` covers `10.1.0.0` through `10.1.255.255`. Convert each address to a 32-bit integer, build the mask from the prefix length, and compare `(addr & mask) == (net & mask)`. Careful with the two extremes: `/32` matches one host and `/0` matches everything, and shifting a 32-bit value by 32 is undefined behaviour in C, so handle `/0` separately. This is the byte-level view of the [IPv4 header](/learn/sec-packets/ipv4) you decoded earlier.

Rule order is policy. Swap two rules and you change the firewall, which is why the [next lesson](/learn/sec-detection/shadowed-rules) audits it.
