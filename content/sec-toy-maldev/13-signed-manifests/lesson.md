---
title: Verify a signed manifest before admission
summary: Separate a manifest that is well formed, one that is authentic and one that is permitted, using Ed25519 from Go's standard library.
minutes: 45
difficulty: hard
tags: security, toy-maldev, signatures, ed25519
id: sec-toy-maldev/signed-manifests
mode: harness
---

This course constructs **inert payload data** for an invented machine. Here the "payload" is just a byte slice and the manifest a small JSON document. Nothing is executed and nothing leaves memory. Everything stays inside the existing Go sandbox.

An earlier lesson showed that a CRC detects accidents and says nothing about who made the data. A **digital signature** answers a different question: was this exact byte string produced by the holder of a private key whose public half I already trust? Go ships Ed25519 in `crypto/ed25519`, and you should not invent your own scheme around it.

Admission needs three separate verdicts, and mixing them up is the classic bug:

- **Valid**: the manifest parses strictly and its fields make sense. A forged manifest can be perfectly valid.
- **Authentic**: a trusted key produced a signature over exactly these bytes. A genuine manifest can ask for something you do not allow.
- **Permitted**: the signed contents satisfy local policy: not expired, and every requested permission allowed. A manifest, even a genuine one, does not grant itself rights.

A fourth check ties the manifest to the artifact: the manifest names the SHA-256 digest of the payload, and the payload you were handed must match it.

Pitfalls worth knowing. Sign and verify the **raw bytes** you received, not a re-encoded copy. Prefix the signed bytes with a fixed **domain separator**, so a signature made for some other purpose cannot be replayed here. Choose the public key from your own trusted key table by ID, never from the manifest. And check key and signature lengths yourself, because `ed25519.Verify` panics on a malformed public key.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.

Limits of the model: there is no key distribution, revocation or rotation here, and the policy table is trusted caller input.
