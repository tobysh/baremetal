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

## Matching a rule

A rule matches on protocol (`tcp`, `udp` or `any`), source and destination network, and destination port. Networks are written as CIDR, an address plus the number of leading bits that must match: `10.1.0.0/16` covers `10.1.0.0` through `10.1.255.255`. Convert each address to a 32-bit integer, build the mask from the prefix length, and compare `(addr & mask) == (net & mask)`. Masking the rule's network too means a non-normalised network such as `10.1.2.3/8` behaves like `10.0.0.0/8`. Careful with the two extremes: `/32` matches one host and `/0` matches everything, and shifting a 32-bit value by 32 is undefined behaviour in C, so handle `/0` separately. This is the byte-level view of the [IPv4 header](/learn/sec-packets/ipv4) you decoded earlier.

```c
uint32_t mask = len == 0 ? 0 : 0xFFFFFFFFu << (32 - len);
int in_net = (addr & mask) == (net & mask);
```

## Worked example

```text
allow tcp any 192.0.2.10/32 443
allow tcp 10.0.0.0/8 192.0.2.0/24 22-23
deny  any any any any
(blank line)
tcp 203.0.113.5 192.0.2.10 443
tcp 10.4.4.4 192.0.2.99 23
udp 10.4.4.4 192.0.2.99 23
icmp 10.4.4.4 192.0.2.10 0
tcp 203.0.113.5 192.0.2.10 80
```

```text
packet 1: allow (rule 1)
packet 2: allow (rule 2)
packet 3: deny (rule 3)
packet 4: deny (rule 3)
packet 5: deny (rule 3)
```

Packet 3 is UDP, so the `tcp` rules skip it and the catch-all denies it. Without the final `deny` rule, packets 3 to 5 would print `deny (default)`, the same decision but a different line, which shows whether a rule or the default decided.

## Pitfalls

- **First match wins.** Stop at the first matching rule; never keep scanning and let a later rule override.
- **Port ranges are inclusive.** `1-1024` matches both 1 and 1024; a single number is a range of one; `any` is 0 to 65535.
- **ICMP.** An `icmp` packet matches only rules whose protocol is `any`.
- **Normalise.** Compare masked values on both sides, or the host bits in `10.1.2.3/8` change the answer.
- **Host order.** Build the 32-bit value with the first octet in the top byte, so that masks of leading bits line up with the dotted notation.

## Why defenders care

Rule order is policy. Swap two rules and you change the firewall, which is why the [next lesson](/learn/sec-detection/shadowed-rules) audits it. A matcher you can run on sample packets lets you test a change before it reaches production.
