---
title: Read a classic pcap container
summary: Count complete pcap records while checking byte order, lengths and the supported link type.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-packets/pcap
mode: harness
---
A capture is a container around frames. Before you can look at a single Ethernet header you have to walk the container correctly, and the container is also attacker-influenced data: a hostile or corrupt file can claim lengths that point far past its own end. This lesson supports only classic microsecond-resolution pcap, version 2.4, Ethernet link type 1. It is not pcapng.

## Layout

```text
Global header (24 bytes)                 Record header (16 bytes) + frame
 0       4     6     8        12       16        20       24
 +-------+-----+-----+--------+--------+---------+--------+
 | magic | maj | min | zone   | sigfig | snaplen | link   |
 +-------+-----+-----+--------+--------+---------+--------+

 0        4        8          12         16
 +--------+--------+----------+----------+--- caplen bytes ---+
 | ts_sec | ts_us  | caplen   | origlen  | frame data         |
 +--------+--------+----------+----------+--------------------+
```

The magic is the byte-order marker. Bytes `d4 c3 b2 a1` mean every later integer is little-endian; `a1 b2 c3 d4` means big-endian. Decide this from the raw bytes first, then decode everything else in that order.

## Worked example

```text
d4 c3 b2 a1  02 00  04 00  00 00 00 00  00 00 00 00  ff ff 00 00  01 00 00 00
80 8c e2 6a  00 00 00 00  2a 00 00 00  2a 00 00 00   <42 frame bytes>
```

The magic says little-endian. Version is `02 00` = 2 and `04 00` = 4. Snaplen `ff ff 00 00` reads as 0x0000ffff = 65535 (read the same bytes big-endian and you get 4294901760, which is why the magic comes first). Link type is 1. In the record, `2a 00 00 00` is 42 for both captured length and original length, so the whole frame was kept. If caplen were 42 and origlen 142, the frame was cut by the snaplen and the file is still valid.

## Pitfalls

- **Byte order.** A field read in the wrong order produces a plausible number, not an error. Version `00 02` read as little-endian is 512.
- **Length arithmetic.** `off + caplen > n` can overflow on a 32-bit size. Compare `caplen > n - off` after proving `off <= n`.
- **Trailing bytes.** Fewer than 16 bytes after the last record is a partial header, which is an error rather than an ignorable tail.
- **Limits.** Microseconds must be below 1,000,000, caplen must not exceed snaplen, and caplen must not exceed origlen.

## The defender's view

A capture file is untrusted input to your analysis tools. Parsers that trust `caplen` have been a steady source of crashes in packet tools, so every length is checked against the bytes you actually hold. When you open `sample.pcap` in Wireshark (display filter `frame`), compare "Frame captured length" with "Frame length": a gap is truncation by the capture, not a parsing fault. All addresses and payloads here are fictional.
