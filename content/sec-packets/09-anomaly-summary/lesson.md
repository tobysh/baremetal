---
title: Summarize a possible scan
summary: Count distinct destination ports per source while excluding SYN-ACKs and repeated probes.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/anomaly-summary
mode: harness
---
A high count of distinct connection attempts can help prioritize traffic review. Counting packets alone is misleading because a lost reply can cause the same SYN to be retransmitted many times. This toy rule counts distinct destination ports per source among flows with SYN set and ACK clear.

For each source, deduplicate the qualifying ports, compare the count with a supplied threshold, then return matching source addresses in numeric order. Other flag bits do not affect the SYN/ACK rule. The input combines all destinations by source; this is a stated simplification, not a full port-scan detector. Threshold zero disables the rule.

This is an observation over the supplied offline summaries. Monitoring agents, authorized scanners, shared addresses and incomplete captures can all produce surprising patterns. Do not label an address malicious from this one count. Equivalent Wireshark filter: `tcp.flags.syn == 1 && tcp.flags.ack == 0`. Statistics > Conversations can help inspect the selected traffic, but a distinct-port count still needs grouping and deduplication. Documentation IPs ensure these examples do not accuse real systems.
