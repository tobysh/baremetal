---
title: AEAD usage rules
summary: Fix a toy authenticated-encryption channel that reuses nonces, decrypts before checking the tag or ignores AAD, then audit a call log for those mistakes.
minutes: 40
difficulty: medium
tags: crypto, aead, nonce, associated-data
id: sec-crypto/aead-rules
mode: harness
---
**Authenticated encryption with associated data (AEAD)**, such as AES-GCM or ChaCha20-Poly1305, hides a message and also detects tampering, in one call. It is much safer than separate encrypt and MAC steps, but only if the caller follows a few rules. Almost every real-world failure of AEAD is a *usage* failure, not a cryptographic one.

- **Never reuse a nonce under the same key.** The nonce does not have to be secret, but it must be unique per message. Reuse a nonce and two ciphertexts can be XORed to cancel the keystream, as in [the nonce-reuse lesson](/learn/sec-crypto/nonce-reuse), and for some AEADs (GCM) it also leaks the authentication key. A counter is a good source if it is never reset, and the sender must stop at the limit instead of wrapping.
- **Verify the tag before you decrypt or use anything.** If you decrypt first, an attacker can send altered ciphertexts and learn from your behaviour, and failed packets must not leave half-decrypted plaintext in a buffer the program later reads.
- **Bind the context with associated data.** The associated data (AAD) is authenticated but not encrypted: a user id, a message type, a protocol version, a file name. If it is left out, a valid ciphertext can be moved to a different place, for example a message sealed for user 7 replayed as if it were for user 9. The receiver must supply the AAD it expects from its own state, and a mismatch makes the tag fail.

The lesson uses a small made-up cipher (a hash-style keystream and tag) so that the whole API fits on a page. It is not secure and must never be used for anything real. The rules are the same for the real ones, and the same discipline of [comparing secrets safely](/learn/sec-crypto/constant-time) applies to the tag check.
