---
title: Construct an inert payload instruction stream
summary: Assemble bounded EMIT, READ, WAIT and HALT instructions into a byte stream with explicit operand lengths.
minutes: 40
difficulty: hard
tags: security, toy-maldev, payload-bytecode
id: sec-toy-maldev/payload-bytecode
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

Malware-development concepts can be studied through a machine small enough to inspect completely. A payload describes behavior; an interpreter decides what that behavior means. Here the behavior is deliberately restricted to text output, reads from supplied data and virtual delays. You are building a bytecode assembler, not CPU instructions.

The instruction stream uses these opcodes:

| Byte | Instruction | Following bytes |
| --- | --- | --- |
| 0 | HALT | none; exactly one, at the end |
| 1 | EMIT | one-byte length, then that many text bytes (0–255) |
| 2 | READ | one-byte length, then a nonempty capability key (1–255 bytes) |
| 3 | WAIT | nonzero uint16 virtual ticks in little-endian order |

Text is a Go byte string; embedded NUL and non-UTF-8 bytes are valid. There are no branches or host addresses. Programs have 1–64 instructions, ending with HALT and no earlier HALT. Fields unused by an opcode must be zero/empty. WAIT changes a counter rather than sleeping.

Lengths count bytes, not characters. The two-byte UTF-8 encoding of a character occupies two bytes even if it displays as one symbol. Binary payloads may contain zero bytes, so neither C-style string length nor a NUL terminator belongs in this format.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
