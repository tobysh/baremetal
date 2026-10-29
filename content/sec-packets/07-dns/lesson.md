---
title: Decode one DNS question
summary: Walk length-prefixed DNS labels without following unsupported compression pointers.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/dns
mode: harness
---
A DNS message has a twelve-byte header. The transaction ID is the first two bytes and QDCOUNT is at bytes 4..5. This subset requires exactly one question. Starting at offset twelve, a name is a sequence of one-byte label lengths followed by label bytes, ending in a zero length. The two 16-bit fields after the name are QTYPE and QCLASS.

Validate every label length (at most 63), every input slice and the output capacity. Join labels with dots while preserving their case. For a root name with no labels, return a single dot. Here the output text limit is 253 characters excluding the terminator; label characters are ASCII letters, digits or hyphens. All other label bytes are rejected by this intentionally small display parser.

Real DNS also uses compression pointers, answers and additional records. A pointer has high bits 11 and must not be mistaken for an ordinary length. Reject it in this exercise; a full decoder would need offset bounds and loop detection. Query and response flags are not interpreted, and trailing answer bytes are ignored. Equivalent filters: `dns` and `dns.qry.name == "notes.example.test"`. The names are fictional .test names, and this code never makes a DNS request.
