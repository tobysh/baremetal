---
title: Byte Order
summary: Networks send the most significant byte first. Write and read 16 and 32 bit integers in big-endian order, whatever your CPU does.
minutes: 15
difficulty: easy
tags: endianness, bytes, binary
id: c-networking/byte-order
mode: harness
---
A network is a stream of **bytes**. A 32-bit integer such as `0x12345678` must be cut into four bytes before it can be sent, and both sides must agree on the order.

- **Big-endian** (network byte order): most significant byte first: `12 34 56 78`.
- **Little-endian** (x86-64, most ARM): least significant byte first: `78 56 34 12`.

If you copy the integer's memory straight into a packet (`memcpy(buf, &x, 4)`), you send your CPU's order, and a machine with the other order reads garbage. Every protocol therefore fixes one order, and the Internet's is **big-endian**.

The portable way to write the bytes is with shifts, because shifts work on *values* and do not care how the CPU stores them:

```c
p[0] = (uint8_t)(v >> 24);   // top byte first
p[1] = (uint8_t)(v >> 16);
p[2] = (uint8_t)(v >> 8);
p[3] = (uint8_t)v;
```

Reading is the reverse: shift each byte back into place and combine them with `|`. Cast to `uint32_t` *before* shifting, or `p[0] << 24` happens in `int` and can overflow.

The C library has `htons`, `htonl`, `ntohs` and `ntohl` for this, but writing your own makes the idea concrete, and you will need the "write at this pointer" form for every header in this course.
