---
title: Descriptor Tables: GDT and IDT Entries
summary: Pack bases, limits, access bytes and handler addresses into the odd bit layouts the CPU demands for GDT and IDT entries, and check them against known values.
minutes: 45
difficulty: medium
tags: cpu, descriptors, interrupts
id: osdev/descriptors
mode: harness
---
The CPU doesn't take function pointers or segment sizes as plain numbers. It reads **tables in memory** whose entries have a fixed binary layout, and the layouts are historical accidents: fields are split across the entry because each CPU generation squeezed in more bits without breaking the old format. Getting one bit wrong gives you a crash with no explanation, so this lesson is about building entries *exactly*.

## The GDT

The **Global Descriptor Table** lists memory segments. In 64-bit mode segmentation is mostly switched off, but the CPU still insists on a valid code segment and data segment, and the table is how privilege levels are expressed. Each entry is 8 bytes:

```
byte 0-1  limit bits 0-15
byte 2-3  base  bits 0-15
byte 4    base  bits 16-23
byte 5    access byte
byte 6    low nibble: limit bits 16-19    high nibble: flags
byte 7    base  bits 24-31
```

### Packing an entry, step by step

The flat code segment `0x00CF9A000000FFFF` is a good one to decode, because it shows every field. Start from the inputs: base `0`, limit `0xFFFFF`, access `0x9A`, flags `0xC`. Slice them into the byte layout:

| Byte | Contents | Value |
|---|---|---|
| 0 | limit bits 0-7 | `0xFF` |
| 1 | limit bits 8-15 | `0xFF` |
| 2 | base bits 0-7 | `0x00` |
| 3 | base bits 8-15 | `0x00` |
| 4 | base bits 16-23 | `0x00` |
| 5 | access | `0x9A` |
| 6 | `(flags << 4) \| (limit >> 16)` | `0xC0 \| 0x0F = 0xCF` |
| 7 | base bits 24-31 | `0x00` |

The CPU is little-endian, so byte 0 is the *least* significant byte of the 64-bit number. Reading the bytes from 7 down to 0 gives `00 CF 9A 00 00 00 FF FF`, which is the value above. If you see the answer come out reversed, you built the bytes in the wrong order.

In C the packing is shifts and masks. The base is split by shifting it down and masking to 8 bits, and the limit contributes its low 16 bits to the first field and its next 4 bits to byte 6. Everything is unsigned: use `uint32_t` or `uint64_t` temporaries so that a shift never sign-extends or truncates unexpectedly.

### Decoding the access byte

Each bit of `0x9A` has a meaning. Write it in binary, `1001 1010`:

```
bit 7  P    = 1   present: the entry is valid
bit 6-5 DPL = 00  privilege level 0 (kernel); 11 would be ring 3 (user)
bit 4  S    = 1   a code or data segment (0 would be a system segment)
bit 3  E    = 1   executable: this is a code segment
bit 2  DC   = 0   conforming / direction, 0 for the simple case
bit 1  RW   = 1   readable (for code) or writable (for data)
bit 0  A    = 0   "accessed": the CPU sets this when the segment is used
```

Now `0x92` is `1001 0010`: the same, except bit 3 is 0, so it is a data segment. `0xFA` is `1111 1010`: the same as `0x9A` with DPL 3, which is how user programs get code and data selectors of their own later.

### Why the layout is so strange

The 8086 had 16-bit segment bases and 20-bit addresses. The 286 added protected mode with a 24-bit base and a 16-bit limit, and the 386 widened both to 32 and 20 bits, *keeping the old bytes where they were* so 286 software still worked. The extra bits had to go in the leftover spaces, which is why the base is scattered across bytes 2, 3, 4 and 7 and the limit's top bits share byte 6 with the flags. Nobody would design it this way today. You write it carefully because the CPU will not forgive a mistake.

## The access byte

The **access byte** says what the segment is: present (bit 7), privilege ring (bits 5-6), "code or data" (bit 4), executable (bit 3), readable/writable (bit 1). Common values: `0x9A` is a ring-0 code segment, `0x92` is a ring-0 data segment, `0xFA` and `0xF2` are the same for ring 3.

The **flags** nibble holds granularity (bit 3: limit counts 4 KiB pages), 32-bit size (bit 2) and the 64-bit code flag (bit 1, "L"). A flat 32-bit code segment is `0xC`; a 64-bit one is `0xA`.

So the flat 32-bit code segment (base 0, limit `0xFFFFF`, access `0x9A`, flags `0xC`) is the 64-bit value `0x00CF9A000000FFFF`. Notice how the limit's top nibble `F` (bit 19:16) shares byte 6 with the flags nibble `C`.

## The IDT

The **Interrupt Descriptor Table** maps interrupt numbers to handler functions. In 64-bit mode each gate is 16 bytes:

```
byte 0-1   handler address bits 0-15
byte 2-3   code segment selector (0x08 for the kernel code segment)
byte 4     IST index (0 for none)
byte 5     type and attributes (0x8E = present, ring 0, interrupt gate)
byte 6-7   handler address bits 16-31
byte 8-11  handler address bits 32-63
byte 12-15 reserved, zero
```

### Splitting a handler address

The same scattering trick applies to the IDT. A 64-bit handler address is cut into a low 16 bits, a middle 16 bits and a high 32 bits, stored in three separate places. Take a handler at `0xFFFFFFFF80101234`:

| Piece | Bits | Value | Stored in |
|---|---|---|---|
| low | 0-15 | `0x1234` | bytes 0-1 |
| middle | 16-31 | `0x8010` | bytes 6-7 |
| high | 32-63 | `0xFFFFFFFF` | bytes 8-11 |

The code segment selector in bytes 2-3 is the **selector**, not an address: it is an index into the GDT. A selector is 16 bits: the top 13 are the table index, bit 2 picks the GDT or an LDT, and the low two are the requested privilege level. Index 1 with those flags clear is `1 << 3 = 0x08`, the kernel code segment from the previous section. That is why the selector in every gate you build is `0x08`: the handler runs under the kernel's code segment.

The type byte `0x8E` is `1000 1110`: bit 7 is *present*, bits 5-6 are the privilege level allowed to trigger the gate with `int` (0 for kernel only), and the low four bits `1110` mean 64-bit interrupt gate (`1111` is a trap gate). Bytes 12-15 are reserved and must be zero, which is why the entry is 16 bytes and not 12.

### What the CPU does with it

When an interrupt with vector `n` arrives, the CPU reads entry `n` from the table, loads the code segment from the selector, saves the old state on the stack, and jumps to the handler address. If the entry is not marked present (an all-zero entry is not), or the vector lies beyond the table's limit, the CPU raises a fault instead. A fault that itself has no valid handler escalates, which is why a bad IDT tends to show up as a sudden reset with no message. The next lesson explains that chain.

The 64-bit handler address is cut into three pieces. An *interrupt gate* (`0x8E`) clears the interrupt flag on entry, a *trap gate* (`0x8F`) does not.

## Telling the CPU where the table is

`lgdt` and `lidt` don't take the table itself, they take a 10-byte **descriptor pointer**: a 16-bit size (in bytes, **minus one**) and a 64-bit base address.

Why "minus one"? The limit field means "the last valid byte offset", not "the size". A table with 3 entries of 8 bytes is 24 bytes, so the last valid byte is at offset 23, and that is the number you give the CPU. It also means a full 64 KiB table (the maximum) fits in the 16-bit field as `0xFFFF`; a plain size of 65,536 would not. For an IDT of 256 gates the limit is `256 * 16 - 1 = 4095`. Forgetting the `- 1` is probably the most common mistake in this whole topic, and it lets the CPU read one entry beyond the table's end.

The pointer has an odd shape: a 2-byte size immediately followed by an 8-byte address, 10 bytes in total, with no padding between. That is another reason the struct must be packed. If the compiler inserted 6 bytes of padding to align the address, `lgdt` would read the wrong bytes.

## Packed structs

The compiler would normally pad a struct so each field is aligned. That would break these layouts, so we mark the structs `__attribute__((packed))`. The starter defines them for you; `sizeof` is checked in the tests.

There are two details to be careful about. First, taking a field of a packed struct by pointer can produce a misaligned pointer, so assign fields directly and do not pass them around by reference. Second, check `sizeof` in your head: the GDT entry is 8 bytes, the IDT gate 16 and the pointer 10. Sizes that disagree with those numbers mean a field is the wrong width.

## How to check your work

You can verify most of these without hardware, by hand. For the GDT, build the entry and compare it with a known value such as `0x00CF9A000000FFFF` (32-bit code), `0x00CF92000000FFFF` (32-bit data) or `0x00AF9A000000FFFF` (64-bit code, flags `0xA`). Note that only the flags nibble differs between the 32-bit and 64-bit code values, which is a good way to see that the **L** bit is what makes the code segment 64-bit. For the IDT, check that stitching the three address pieces back together gives the original address. These are exactly the kind of "known answer" tests that real kernel projects use before ever booting.

## Your turn

Implement the three functions. The tests build real entries and compare them with the values the CPU manuals and OS projects use.
