---
title: Layout and Relocations
summary: Assign addresses to sections, then patch absolute and PC-relative fields so each reference points at its final location.
minutes: 60
difficulty: hard
tags: linker, relocations, layout
id: go-executable-formats/layout-and-relocation
mode: harness
---
With names resolved, the linker decides **where everything goes** (layout) and then patches every place that mentions an address (relocation).

## Outcome

Given parsed objects, you compute a memory plan and produce the final text and data bytes with all relocations applied, or a precise error.

## Layout rules

All text sections are placed one after another starting at `Base` (0x1000), in object order then section order, each aligned to 4 bytes. The data segment starts at the next `PageSize` (0x100) boundary after the text. Inside it come all data sections (aligned to 4), then all bss sections (aligned to 4). Bss occupies memory but no file bytes, so the data segment's memory size is larger than its file size. The whole image must end at or below 1 MiB (`ErrTooBig`).

## Relocation arithmetic

A relocation names a four-byte field, a target symbol with final address **S**, an addend **A** and the field's own address **P**:

- `RelAbs32`: field = `S + A` (an absolute address, like a pointer in data or `LI r0, &counter`)
- `RelRel32`: field = `S + A - (P + 4)` (a distance from the end of the field, which is where the machine's program counter points after reading it)

The old bytes in the field are ignored; the addend lives in the relocation record (the "RELA" style). Results must fit: an absolute value in `0..2^32-1` and a relative one in a signed 32-bit integer, else `ErrRelocRange`. A call is a good mental model: the compiler emitted `CALL ????` with the target unknown; you compute how far it is.

Mistakes here are the classic ones: forgetting alignment padding, using the object-relative offset where a final address is needed, and off-by-four in the PC-relative formula. The tests compare your bytes with a reference linker model written independently from the rules above, over randomly sized inputs, and check addends in both directions.
