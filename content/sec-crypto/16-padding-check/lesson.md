---
title: Validate block padding evenly
summary: Check PKCS#7 padding on a decrypted block so every pad byte is inspected whatever the content, the discipline that closes padding oracles.
minutes: 30
difficulty: medium
tags: security, crypto-misuse, constant-time
id: sec-crypto/padding-check
mode: harness
---
Block ciphers work on fixed-size blocks, so a message is padded to a multiple of the block size. PKCS#7 padding appends `p` bytes that each hold the value `p`, where `p` is between 1 and the block size. After decryption the receiver reads the last byte, checks that many bytes, strips them and carries on.

If the receiver reveals which check failed (a different error message, or even a slightly different response time for a bad count than for a bad pad byte), an attacker who can submit modified ciphertexts learns something about the plaintext from each answer. That leak is called a padding oracle, and it has broken real protocols without the cipher itself being weak. Two defences are the standard ones: authenticate the ciphertext first and reject forged messages before decrypting anything, and make the padding check treat all failures alike.

Here you write the check. Inspect the entire final block with the same amount of work for every input of valid shape, accumulate a single bad flag rather than returning early, and report only pass or fail. The optional `work` output counts bytes examined so a test can verify the discipline; it is an accounting aid, not a timing measurement. The blocks are synthetic.
