---
title: Walk a record stream
summary: Walk a synthetic TLS-style record stream in a growing buffer: validate each header, count complete records, and report when more bytes are needed.
minutes: 35
difficulty: medium
tags: security, protocol-analysis, parsing
id: sec-packets/record-stream
mode: harness
---
TCP delivers a stream of bytes, not messages, so a protocol on top of it has to find its own record boundaries. TLS records do it with a five-byte header followed by that many bytes of body. A receiver collects bytes as they arrive and keeps asking the same question of the buffer: is there a complete, valid record at the front?

## Layout

```text
 0      1      2      3      4       5 ...
 +------+------+------+------+------+------------------+
 | type | ver  | ver  | length (BE) | body (length B)  |
 | 20-23|  3   | 1..4 | hi    lo    |                  |
 +------+------+------+------+------+------------------+

 buffer:  [record][record][record][partial ...
 pos:      0       r1      r2      r3        n

 walk: rem = n - pos
   rem == 0              -> 0  clean end
   rem < 5               -> 1  need more (partial header)
   header invalid        -> -1 stop reading
   length > rem - 5      -> 1  need more (partial body)
   otherwise: count it, pos += 5 + length
```

Valid here means type 20 to 23, major version 3, minor 1 to 4, and length 1 to 16384. A header can be judged as soon as all five bytes are present, before the body arrives.

## Worked example

```text
17 03 03 00 03 aa bb cc   16 03 03 00 10 01 02 ...(only 4 of 16 body bytes)
```

The first record is type 23, version 3.3, length 3, and its body is `aa bb cc`: 8 bytes in all, so it is counted and `pos` becomes 8. The next header is type 22, length `00 10` = 16, but only 4 body bytes are present, which is `length > rem - 5`. The walk returns 1 with `records = 1` and `consumed = 8`: the caller keeps the unread bytes and waits. If the second header started `99 03 03 ...`, 0x99 is not a valid type, and the walk returns -1 immediately, even with the body still missing.

## Pitfalls

- **Three answers.** A clean boundary (0), "wait for more" (1) and "invalid, stop" (-1) call for three different reactions. Treating "need more data" as an error drops valid connections; treating "invalid" as "need more" makes a receiver buffer attacker data forever.
- **Overflow.** Write `length > rem - 5` after showing `rem >= 5`, never `pos + 5 + length > n`, which can wrap for large values.
- **Header before body.** Judge the header the moment five bytes exist. A bad length of 0 or 16385 is an error without waiting.
- **Consumed bytes.** Report only complete records, so the caller can discard exactly that many bytes. A partial record stays in the buffer.
- **Byte order.** Length is big-endian: `b[3] << 8 | b[4]`.

## The defender's view

Bound the buffer per connection and the record size, so a peer cannot make you hold unbounded data. Reject malformed headers at once rather than waiting for a body that will never make sense. Parser bugs in length handling have been a recurring source of vulnerabilities in record layers. The bytes here are synthetic and carry no session or key material.
