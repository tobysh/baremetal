---
title: Validate a capsule before interpreting it
summary: Check container integrity, decode every instruction and reject entry offsets inside operands.
minutes: 40
difficulty: hard
tags: security, toy-maldev, validated-loader
id: sec-toy-maldev/validated-loader
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

A loader is a trust boundary. Header fields are untrusted, the checksum is not a signature, and an entry offset can point into the middle of an operand. Validate the whole image before returning any instructions.

The entry field is a byte offset in the capsule; the interpreter wants an instruction index. Build a map of the instruction boundaries while decoding. Even instructions before entry must be structurally valid. Later lessons distinguish structural validity from whether a requested operation is permitted.

The capsule format is invented for this course; it is distinct from the analysis course's BMX files.

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | ASCII magic `BML1` |
| 4 | 2 | entry **byte offset** into code, little-endian uint16 |
| 6 | 2 | code byte length, little-endian uint16 |
| 8 | 4 | IEEE CRC32 of code bytes, little-endian uint32 |
| 12 | length | code bytes |

Code length is 1–4096; entry must be smaller than length. No trailing bytes are allowed. CRC32 detects accidental corruption; an attacker can recompute it, so it does not establish authenticity.

The instruction stream uses these opcodes:

| Byte | Instruction | Following bytes |
| --- | --- | --- |
| 0 | HALT | none; exactly one, at the end |
| 1 | EMIT | one-byte length, then that many text bytes (0–255) |
| 2 | READ | one-byte length, then a nonempty capability key (1–255 bytes) |
| 3 | WAIT | nonzero uint16 virtual ticks in little-endian order |

Text is a Go byte string; embedded NUL and non-UTF-8 bytes are valid. There are no branches or host addresses. Programs have 1–64 instructions, ending with HALT and no earlier HALT. Fields unused by an opcode must be zero/empty. WAIT changes a counter rather than sleeping.


Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
