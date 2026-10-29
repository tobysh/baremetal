---
title: Find reused nonces
summary: Scan a synthetic encryption log for a key and nonce pair that was used for more than one message, the mistake that breaks stream ciphers.
minutes: 25
difficulty: medium
tags: security, crypto-misuse, log-analysis
id: sec-crypto/nonce-reuse
mode: stdout
---
Stream ciphers and counter-mode ciphers turn a key and a nonce (a number used once) into a keystream that is XORed with the message. Reuse the same pair for two messages and both ciphertexts were masked with the same keystream. XORing them cancels the keystream and leaves the XOR of the two plaintexts, which is exactly what the [repeating-XOR exercises](/learn/sec-crypto/repeating-xor) showed how to attack. Nothing about the key needs to be guessed.

The rule is therefore about the pair, not the nonce alone. The same nonce under two different keys is harmless, and the same message sent twice because of a retransmission is not a second use. What matters is two different messages under one key and one nonce.

Your program audits a log. Each line has the shape `key=NAME nonce=HEX msg=ID`. Compare nonce text without regard to letter case, since upper and lower case hex name the same number. Lines that do not match the shape or whose nonce is not hexadecimal are ignored. Report each pair with two or more distinct message ids. In real code, avoid the problem by construction: use a random nonce of the length the cipher requires, a never-repeating counter, or an AEAD construction from a vetted library that handles it for you. Everything here is made-up data.
