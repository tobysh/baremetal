---
title: Parsing an Object File
summary: Decode sections, symbols and relocations from untrusted TOBJ bytes, checking every offset, index and name before use.
minutes: 45
difficulty: medium
tags: binary-formats, parsing, validation
id: go-executable-formats/parsing-an-object
mode: harness
---
An **object file** is a compiler's output before linking: chunks of code and data (**sections**), names for places inside them (**symbols**), and notes saying "patch these four bytes once you know where that symbol ends up" (**relocations**). This lesson parses all three tables from bytes. The header parser from the previous lesson is provided.

## Outcome

`ParseObject` returns a structured `Object` for valid input and a wrapped error for every kind of invalid input, with no panic for any byte string.

## Validate before use

Every number in the file is a claim. Names are offsets into the string table and must point inside it at a NUL-terminated string. Section contents are `offset` and `size` pairs that must lie after the tables and inside the file. A symbol's section index must exist and its value must not lie past the section's end. A relocation must patch four bytes **inside** a text or data section and name an existing symbol. Padding bytes the format reserves must be zero, otherwise two files that mean the same thing could differ and a future version could not extend them.

Two habits make this manageable. Do arithmetic on file-provided values in `uint64`, and **copy** the bytes you keep: if a section's `Data` aliases the input, the next buffer reuse silently rewrites your parsed object.

The tests damage a valid object one byte at a time and cut it at every length. They accept an error or a structurally valid object, never a panic, and they check that anything accepted satisfies the invariants a later stage will rely on (indexes in range, sizes consistent). They cannot prove your validation is *complete*; they sample single-byte damage and a list of known rules.

The format is **invented for this course** (named TOBJ for objects and TEXE for executables). It borrows ideas from ELF and a.out but is far smaller, so every byte can be accounted for. It shows how linkers work, not how any real platform loads programs. Real formats add alignment rules, dynamic linking, position independence and much more.
