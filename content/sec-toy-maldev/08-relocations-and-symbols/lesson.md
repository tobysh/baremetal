---
draft: true
title: Link named functions and patch relocations
summary: Combine invented object fragments, resolve exported names and patch CALL operands with checked offsets into a versioned capsule.
minutes: 50
difficulty: hard
tags: security, toy-maldev, linking, relocations
id: sec-toy-maldev/relocations-and-symbols
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

Real toolchains build programs from separately produced pieces. A compiler leaves a placeholder where a call target is not yet known, and a linker later fills it in using a **symbol table** (names to locations) and a **relocation list** (places to patch). Both lists come from untrusted object files, so a linker is a parser: every offset, name and patch site must be checked before a single byte is written.

This lesson builds a tiny linker for the invented payload format. Nothing is executed: the linker only produces bytes and this lesson never interprets the new instruction. The point is the bookkeeping, and the way a format grows without breaking older readers.

## Format version 2

Lessons 1 to 7 use format version 1, which has four opcodes and exactly one HALT at the end. Version 2 makes two changes:

- A new opcode `CALL` (byte 4) followed by a little-endian uint16 **absolute byte offset** into the linked code. In an object file that operand is a placeholder and must be `00 00`.
- Code may contain more than one HALT, because each linked function ends with its own HALT. HALT still has no operands.

A capsule with the magic `BML1` is version 1 and a capsule with the magic `BML2` is version 2. The layout is the one you know: magic, entry byte offset (uint16), code length (uint16), IEEE CRC32 of the code (uint32), code. A version-1 reader rejects `BML2` by its magic, so it never misreads CALL as something else. The linker therefore emits the **lowest version that can express the result**: `BML1` when the linked code has no CALL, exactly one HALT (the last instruction) and so would be accepted by a version-1 loader, otherwise `BML2`. A program that did not need version 2 must keep producing version-1 bytes.

## Objects

An object is a fragment of code plus two lists. Exports map a name to the byte offset of an instruction inside this object's code. Relocations name the offset of a CALL operand and the symbol it must point at.

Linking concatenates the objects in the order given. An object's base is the total code length before it. The patched value is the **linked** offset `base + exportOffset` of the symbol, not the object-local one.

## What is trusted

Nothing in an object. Export offsets, relocation sites and sizes are all caller-supplied data. A relocation site that falls outside the code, or inside another instruction's operand, would otherwise let a malformed object overwrite arbitrary bytes. In memory-unsafe languages that is a write primitive; here it would silently corrupt instructions. Rejecting it is the whole job.

A linker that checks only that a site is inside the buffer still lets an object patch the middle of a text operand. Check that the site is exactly the operand of a CALL.

A simulation shows its stated model. This linker has one global namespace and no versioning of symbols, weak symbols or section alignment, which real linkers handle. It exists to show where the checks belong.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
