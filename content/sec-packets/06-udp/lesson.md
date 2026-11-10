---
title: UDP datagram bounds
summary: Use the UDP length field to separate its payload from trailing captured bytes.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-packets/udp
mode: harness
---
UDP is the simplest transport header: four fields and no connection. That simplicity hides one trap. The datagram carries its own length field, and it is yours to check against the bytes you were given, because the capture span may contain more bytes than the datagram (padding) or fewer (truncation).

## Layout

```text
 0           2           4           6           8
 +-----------+-----------+-----------+-----------+------ payload ------
 | src port  | dst port  | length    | checksum  | (length - 8 bytes)
 +-----------+-----------+-----------+-----------+----------------------
   2 bytes     2 bytes     header + payload        0 = not computed (IPv4)
```

Every field is two bytes, big-endian. Length counts the 8-byte header plus the payload, so the smallest valid value is 8 (an empty datagram) and the payload size is `length - 8`.

## Worked example

```text
00 35 c3 50  00 0b 00 00  61 62 63
```

`00 35` is source port 53, `c3 50` is destination port 50000 and `00 0b` is length 11. The checksum `00 00` means the sender did not compute one, which IPv4 permits. The payload is 11 - 8 = 3 bytes, `61 62 63`. If this packet had arrived with 14 bytes in the span, 3 trailing bytes would be Ethernet padding, not payload, and the answer is still 3. If length were `00 0c` (12) in an 11-byte span, the datagram claims a byte that was never captured, so it must be rejected.

## Pitfalls

- **Read the length after checking `n >= 8`.** A 7-byte span has no length field.
- **Lower bound.** A length of 0 to 7 is impossible because the header alone is 8 bytes. Without this check `length - 8` wraps to a huge `size_t`.
- **Upper bound.** `length <= n`. The payload comes from the declared length, not from `n`.
- **Byte order.** `00 0b` is 11. Read little-endian, it is 2816 and the parse would reject a good packet.
- **Checksums.** Verifying one needs the IPv4 pseudo-header, so it is a separate step that this lesson skips.

## The defender's view

A port number is only a hint: UDP port 53 usually carries DNS, but you only know once the application bytes parse. Because UDP has no handshake, source addresses are easy to forge, which is why reflection and amplification attacks exist, and why a monitor should treat a small request that provokes a large reply as worth rate limiting. Parsers that trust the declared length against the captured length are the other half of the story: check both. Filters: `udp` and `udp.port == 53`. All data in this track is synthetic.
