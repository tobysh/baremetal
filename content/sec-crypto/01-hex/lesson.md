---
title: Bytes as hex
summary: Turn raw bytes into hexadecimal text and back, the format every crypto tool uses to show binary data.
minutes: 20
difficulty: easy
tags: hex, encoding, bytes
id: sec-crypto/hex
mode: harness
---
Encrypted data is not text. It is a pile of arbitrary bytes, and many of them are unprintable. To look at it, paste it into a bug report or send it in JSON, we **encode** it as text. The simplest encoding is **hexadecimal**: each byte (0 to 255) becomes exactly two characters from `0123456789abcdef`.

```
byte 0x1a  ->  "1a"
byte 255   ->  "ff"
byte 0     ->  "00"
```

One byte holds 8 bits, and one hex digit holds 4 bits, so a byte is two **nibbles**: the high one (`b >> 4`) and the low one (`b & 0x0f`). A nibble is a number 0 to 15, and you turn it into a character with a lookup string:

```c
static const char DIGITS[] = "0123456789abcdef";
char hi = DIGITS[b >> 4];
char lo = DIGITS[b & 0x0f];
```

Going back is the reverse: find the value of each character (`'a'` is 10, `'F'` is 15) and combine two of them with `(hi << 4) | lo`.

## Encoding is not encryption

Hex and base64 are just different spellings of the same bytes. Anyone can undo them without a key. Beginners sometimes "protect" a password by base64-encoding it. That protects nothing. Encoding is about *representation*; encryption is about *secrecy*. Keep the two apart in your head: the rest of this course relies on it.

## Bad input

Data you decode comes from outside, so it can be wrong: an odd number of digits, or a character like `z`. A decoder must notice and report failure instead of producing garbage. Real parsers that skip this step have caused real vulnerabilities.
