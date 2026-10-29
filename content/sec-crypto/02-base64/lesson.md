---
title: Base64
summary: Pack three bytes into four printable characters, and see how padding works.
minutes: 25
difficulty: medium
tags: base64, encoding, bits
id: sec-crypto/base64
mode: harness
---
Hex doubles the size of your data: 1 byte becomes 2 characters. **Base64** is denser. It splits the data into 6-bit pieces instead of 4-bit ones, and each piece (0 to 63) picks one character from an alphabet of 64:

```
ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789 + /
```

Three bytes are 24 bits, which is exactly four 6-bit pieces. So base64 turns every 3 bytes into 4 characters, a 33% overhead instead of hex's 100%.

```
bytes   M        a        n
bits    01001101 01100001 01101110
pieces  010011 010110 000101 101110
index   19     22     5      46
chars   T      W      F      u        -> "TWFu"
```

In C the pieces come out with shifts and masks:

```c
uint32_t v = (a << 16) | (b << 8) | c;
idx0 = (v >> 18) & 63;   idx1 = (v >> 12) & 63;
idx2 = (v >> 6)  & 63;   idx3 =  v        & 63;
```

## Padding

If the input is not a multiple of 3 bytes, the last group is short. One leftover byte (8 bits) gives two pieces and the output is filled up with `==`; two leftover bytes (16 bits) give three pieces and one `=`. The `=` characters tell the decoder how many bytes were really there.

```
"f"      -> "Zg=="
"fo"     -> "Zm8="
"foo"    -> "Zm9v"
```

## Why it exists

Base64 exists so binary data can travel through systems that only handle text: email, JSON, URLs, PEM certificate files, `Authorization: Basic ...` headers. Like hex, it is **not encryption**. `Authorization: Basic dXNlcjpwYXNz` is just `user:pass` in a thin disguise.
