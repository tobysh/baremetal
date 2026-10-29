---
title: XOR with a repeating key
summary: Encrypt a message by XORing it with a key that repeats, and meet the cipher you will break in the next lesson.
minutes: 20
difficulty: easy
tags: xor, bits, stream-cipher
id: sec-crypto/repeating-xor
mode: stdout
---
**XOR** (`^` in C) compares two bits and gives 1 when they differ:

```
0 ^ 0 = 0     0 ^ 1 = 1     1 ^ 0 = 1     1 ^ 1 = 0
```

Two facts make it the heart of cryptography. First, XOR with the same value twice cancels out: `(m ^ k) ^ k == m`. Second, if `k` is random, then `m ^ k` looks random too and tells you nothing about `m`.

So the simplest cipher is: XOR each message byte with a key byte. Encrypting and decrypting are the same operation.

```c
unsigned char c = 'A' ^ 0x2b;   // encrypt
unsigned char p = c ^ 0x2b;     // decrypt: 'A' again
```

## A short key

A key as long as the message, used once and truly random, is the **one-time pad**, and it is provably unbreakable. But then you must share a key as big as everything you will ever send. So people try a short key that **repeats**: byte `i` of the message is XORed with `key[i % key_length]`.

```
message  B  u  r  n
key      I  C  E  I   <- the key "ICE" repeats
```

That is the **Vigenere** idea in binary, and it is weak. The repetition leaks patterns, and the next lessons show how an attacker recovers the key from nothing but ciphertext. Reusing a key also breaks the one-time pad: if two messages are XORed with the same key, then `c1 ^ c2 == m1 ^ m2`, and the key has vanished from the equation. This "two-time pad" mistake has sunk real systems.

## Printing bytes as hex

The ciphertext bytes are mostly unprintable, so print each one as two lowercase hex digits with `printf("%02x", byte)`.

Real systems never use a repeating XOR. They use a vetted stream cipher such as ChaCha20, which derives a pseudorandom keystream from a secret key and nonce. The nonce must follow the construction's uniqueness requirements. Never roll your own crypto in production.
