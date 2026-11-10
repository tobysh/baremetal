---
title: Summarize a possible scan
summary: Count distinct destination ports per source while excluding SYN-ACKs and repeated probes.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/anomaly-summary
mode: harness
---
A scan asks many different questions quickly: is this port open, and this one, and this one? One way to notice it in a capture is to count how many distinct destination ports each source has tried to open. Counting raw packets is misleading, because a single lost reply makes an ordinary client retransmit its SYN several times.

## Layout

```text
Input: one summary per flow
 +-------------+----------+-------+
 | src (u32)   | port u16 | flags |   flags is the raw TCP flag byte
 +-------------+----------+-------+

Qualifying flow:   (flags & 0x12) == 0x02
                    ACK 0x10 clear, SYN 0x02 set; other bits ignored

 flags 0x02 SYN          -> counts       flags 0x12 SYN+ACK  -> excluded
 flags 0x42 SYN+ECE      -> counts       flags 0x10 ACK      -> excluded
 flags 0x04 RST          -> excluded     flags 0x03 SYN+FIN  -> counts
```

Group by `src`, drop duplicate `(src, port)` pairs, compare each count with the threshold, and output the sources that reach it in ascending numeric order.

## Worked example

```text
src            port  flags   counts?
192.0.2.10      80   0x02    yes   -> ports {80}
192.0.2.10      80   0x02    duplicate, no change
192.0.2.10     443   0x02    yes   -> ports {80, 443}
192.0.2.10     22    0x12    SYN-ACK, a reply, ignored
198.51.100.7    22   0x02    yes   -> ports {22}
```

With a threshold of 2, only 192.0.2.10 qualifies: two distinct qualifying ports, and the repeated SYN to port 80 and the SYN-ACK to port 22 do not add to the count. 198.51.100.7 has one. With threshold 3, nobody qualifies. With threshold 1 both qualify, and the sorted output lists 192.0.2.10 (0xc000020a) before 198.51.100.7 (0xc6336407).

## Pitfalls

- **Mask, not equality.** `flags == 2` misses SYN with ECE or CWR set. `flags & 2` includes SYN-ACK. The right test is `(flags & 0x12) == 0x02`.
- **Deduplicate per source.** The same port from two sources counts once for each of them, not once overall.
- **Truncate after sorting.** If `max` is smaller than the result, the caller expects the lowest addresses, so sort before limiting.
- **Unsigned order.** Compare `uint32_t` values, not signed ints; addresses over 128.0.0.0 would sort first as negatives.
- **Zero cases.** Threshold 0, `n == 0` or `max == 0` return 0 and write nothing.

## The defender's view

This is a triage rule, not a detector. Authorised vulnerability scanners, monitoring systems, shared NAT addresses and half-complete captures all produce the same counts as an intruder. Real systems add time windows, per-destination counts and allow-lists, and they treat the result as a prompt to look closer, not a label. Filter: `tcp.flags.syn == 1 && tcp.flags.ack == 0`, then Statistics > Conversations. The addresses used here are documentation ranges.
