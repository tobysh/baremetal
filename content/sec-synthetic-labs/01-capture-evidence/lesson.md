---
title: Phase 1: reconstruct capture evidence
summary: Follow TCP handshakes, HTTP headers and DNS queries in a JSON capture dump.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, capture-evidence
id: sec-synthetic-labs/capture-evidence
mode: harness
---

A capture is a sequence of observations, not a list of conclusions. A client SYN, reverse SYN-ACK and client ACK establish a TCP flow. Match both addresses and ports; another client's ACK does not complete it. Later HTTP requests on that flow may carry evidence.

This lesson ships a normalized JSON dump of a fictional capture. `proto` is `TCP` or `DNS`; TCP flags are exactly `S`, `SA`, `A`, or `PA`. `payload` contains a complete HTTP request (stream reassembly is outside this exercise). DNS records use `query`. HTTP header names are case-insensitive. A POST request with `X-Lab-Transfer: archive` identifies the fictional transfer. Report its destination port only after a completed handshake. Ignore unrelated requests and incomplete flows. Collect unique DNS query names in sorted order. A production analyzer also needs sequence numbers, retransmission handling, IP fragmentation and TCP reassembly.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
