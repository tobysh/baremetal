---
title: Decode one DNS question
summary: Walk length-prefixed DNS labels without following unsupported compression pointers.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/dns
mode: harness
---
DNS names are the one place in this track where a length byte is followed by variable data, then by another length byte, and so on. Every step is a chance to read past the buffer, so the whole parser is built around one rule: before using a length, prove the bytes exist. This lesson decodes the first question of a message.

## Layout

```text
Header (12 bytes)
 0      2       4        6        8        10       12
 +------+-------+--------+--------+--------+--------+
 | id   | flags | QDCOUNT| ANCOUNT| NSCOUNT| ARCOUNT|
 +------+-------+--------+--------+--------+--------+
Question, starting at byte 12
 +----+-------+----+-------+----+---+-------+--------+
 | 05 | notes | 07 |example| 04 |tes| 00    | QTYPE  | QCLASS |
 +----+-------+----+-------+----+---+-------+--------+
 label length, then that many bytes; a zero length ends the name
```

A length byte below 64 is a label length. The top two bits `11` (a byte of 192 or more) start a compression pointer, and `01` and `10` are reserved. The 16-bit QTYPE and QCLASS follow the final zero.

## Worked example

```text
12 34 01 00 00 01 00 00 00 00 00 00
05 6e 6f 74 65 73  07 65 78 61 6d 70 6c 65  04 74 65 73 74  00  00 01  00 01
```

Transaction ID `12 34` and QDCOUNT 1. At offset 12, `05` says five label bytes (`notes`), `07` seven (`example`), `04` four (`test`), then `00` ends the name. The display form is `notes.example.test`: 16 label bytes plus two dots make 18 characters. QTYPE `00 01` is A and QCLASS `00 01` is IN. A name that is just `00` is the root, shown as `.`.

## Pitfalls

- **Length first.** For each label, check `len <= remaining` before copying, then check output capacity. Forgetting either gives an over-read or a buffer overflow.
- **Limits.** Labels are at most 63 bytes and the dotted name at most 253 characters. A 63-byte label next to others is the usual boundary test.
- **Pointers.** A byte of 0xc0 is not "length 192". Real decoders follow the pointer with an offset check and a loop counter; this lesson rejects every length of 64 or more.
- **Missing end.** If the buffer ends before the zero byte, or before the four type and class bytes, the message is truncated.
- **Characters.** The wire format allows any byte in a label; this parser accepts only letters, digits and hyphens so it can safely print the result.

## The defender's view

DNS is parsed by resolvers, firewalls and loggers, and compression loops and oversize names have caused real crashes. Long random-looking labels or very many queries to one domain can indicate tunnelling, but the evidence is in counts and entropy across many messages, not in one question. Use `dns` and `dns.qry.name == "notes.example.test"` in Wireshark. The names here are fictional `.test` names and nothing is ever resolved.
