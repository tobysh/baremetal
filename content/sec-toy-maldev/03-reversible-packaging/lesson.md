---
title: Package and recover a synthetic payload
summary: Implement reversible XOR packaging and reject corrupt or ambiguous envelopes.
minutes: 40
difficulty: hard
tags: security, toy-maldev, reversible-packaging
id: sec-toy-maldev/reversible-packaging
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

A package changes how bytes are stored. It does not automatically make the underlying behavior private or trustworthy. This deliberately transparent envelope stores its one-byte XOR key in the header. Anyone can decode it. The checksum can detect an accidental change, but it is not a signature.

You implement both the authoring and recovery sides. This supports the later loader pipeline while making the limits of reversible encoding explicit. It does not attempt to bypass security tools or conceal executable host code.

The package envelope is `BX01` (4 bytes), decoded length (2 little-endian bytes), nonzero XOR key (1 byte), IEEE CRC32 of decoded bytes (4 little-endian bytes), then each decoded byte XORed with the key. Header size is 11. Decoded length must be 1–8192 and match the envelope length exactly.


Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
