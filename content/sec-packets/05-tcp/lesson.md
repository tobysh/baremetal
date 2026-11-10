---
title: Read TCP segments
summary: Decode ports, sequence numbers and flags while respecting the TCP data offset.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/tcp
mode: harness
---
TCP presents a reliable byte stream to applications, but a capture contains individual segments, and each one is a header whose length you have to compute before you can find the data. This lesson decodes the fixed fields and the payload length; it does not reassemble anything.

## Layout

```text
 0       2       4              8              12   13     14      16     18
 +-------+-------+--------------+--------------+----+------+-------+------+------+
 | src   | dst   | sequence     | acknowledge  |off | flags| window| cksum| urg  |
 | port  | port  | number       | number       |res |      |       |      | ptr  |
 +-------+-------+--------------+--------------+----+------+-------+------+------+
 options (if offset > 5) from byte 20, then the payload at offset*4

 byte 12: [data offset 4 bits][reserved/NS 4 bits]
 byte 13: CWR 0x80  ECE 0x40  URG 0x20  ACK 0x10  PSH 0x08  RST 0x04  SYN 0x02  FIN 0x01
```

All integers are big-endian. The data offset counts four-byte words, so the header is `(p[12] >> 4) * 4` bytes: 20 with no options, up to 60.

## Worked example

```text
9c 40 00 50  00 00 03 e8  00 00 00 00  50 02 10 00  00 00 00 00
```

Source port `9c 40` is 40000 and destination `00 50` is 80. Sequence `00 00 03 e8` is 1000 and the acknowledgement is 0. Byte 12 is `50`: offset 5, a 20-byte header. Byte 13 is `02`, only SYN, so this is a connection opener with no data: payload = 20 - 20 = 0. If the same segment arrived as 25 bytes, 5 bytes would be payload. If byte 12 were `80`, the header would be 32 bytes and the payload would start at 32.

## Pitfalls

- **Use the nibble.** `p[12] * 4` is wrong: the lower four bits are reserved flags. Shift first.
- **Bounds.** The offset must give a header of at least 20 and at most `n`. Offsets 0 to 4 are impossible, and offset 15 needs 60 bytes.
- **Payload.** It is `n - header`, where `n` is the TCP span the IP layer allowed. If the caller passed extra Ethernet padding, the length is wrong, which is why IP total length matters.
- **Raw flags.** Keep byte 13 whole; testing with a mask such as `& 0x12` lets you tell SYN from SYN-ACK.
- **Wraparound.** Sequence numbers are 32 bits and wrap at 2^32, so compare them with care. This lesson only reads them.

## The defender's view

A SYN with ACK clear opens a connection, but one segment is neither a completed handshake nor an attack. Floods, scans and half-open connections all begin with the same flag combination, and a monitor tells them apart by counting across many segments. Odd flag combinations such as SYN together with FIN or no flags at all are worth noting, and should still never crash the parser. Display filters: `tcp` and `tcp.flags.syn == 1 && tcp.flags.ack == 0`. The addresses and ports here are synthetic.
