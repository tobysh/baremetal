---
title: Virtual Memory: Walking Page Tables
summary: Split a virtual address into four table indices, walk the tables the CPU walks, handle huge pages, and work out the effective permissions of a mapping.
minutes: 50
difficulty: hard
tags: memory, paging
id: osdev/page-tables
mode: harness
---
Your kernel has been running in a world of **virtual addresses** since the first instruction: the boot stub turned paging on to enter 64-bit mode. Every pointer you dereference goes through the CPU's memory management unit (the **MMU**), which translates it into a **physical** address using tables that the kernel builds. This is how each program gets a private address space, how the kernel hides its own memory, and how unmapped pages (like address 0) can fault.

## Four levels

A virtual address has 48 meaningful bits. The CPU splits it like this:

```
 63      48 47    39 38    30 29    21 20    12 11         0
+----------+--------+--------+--------+--------+------------+
| sign ext | PML4   | PDPT   | PD     | PT     |   offset   |
|          | 9 bits | 9 bits | 9 bits | 9 bits |   12 bits  |
+----------+--------+--------+--------+--------+------------+
```

Each of those four tables is one 4 KiB frame holding **512 entries of 8 bytes**. The 9-bit field picks the entry. The CPU starts at the table whose physical address is in register **CR3**, picks entry `PML4[bits 47:39]`, which points to a PDPT, picks `PDPT[bits 38:30]`, which points to a PD, then `PD[bits 29:21]` points to a PT, and `PT[bits 20:12]` is the final entry holding the physical frame. The low 12 bits are the offset inside that frame.

The "levels" are numbered 4 (PML4) down to 1 (PT).

Bits 63:48 must be copies of bit 47. Addresses that break that rule are **non-canonical** and always fault.

## An entry

```
 63   62      52 51                12 11  8 7   6  5   2   1   0
+----+----------+--------------------+-----+---+---+---+---+---+---+
| NX | (avail)  | physical address   |     | PS|...|...| U | W | P |
+----+----------+--------------------+-----+---+---+---+---+---+---+
```

- bit 0 **P**resent: if clear, the access faults.
- bit 1 **W**ritable.
- bit 2 **U**ser: ring 3 may access.
- bit 7 **PS** (page size): at level 3 or 2, this entry maps a **huge page** directly instead of pointing to a lower table.
- bit 63 **NX**: no-execute.
- bits 51:12: the physical address of the next table, or of the final frame.

Mask the address with `0x000FFFFFFFFFF000` to separate it from the flags.

## Huge pages

A level-2 entry with PS set maps a **2 MiB** page: the walk stops there, and the physical address is the entry's address bits 51:21, plus the low 21 bits of the virtual address as the offset. A level-3 entry with PS set maps **1 GiB**: address bits 51:30, plus a 30-bit offset.

## Effective permissions

A mapping is writable only if **every** level on the walk allows writing. It's accessible to user mode only if **every** level has the user bit. But NX works the other way: if **any** level has NX set, the page is not executable.

## The tables are just memory

Because the kernel is identity-mapped, a physical address like `0x5000` is also a valid pointer to the same bytes. To read a table whose physical address is `phys`, cast: `(uint64_t *)(uintptr_t)phys`. (Kernels with a different layout use a function here, `phys_to_virt`.)

## A worked walk

Follow the address `0xFFFF800000201234` through a set of tables, by hand.

**Split it.** In binary the interesting bits are: bits 47 to 39 are `1_0000_0000` (256), bits 38 to 30 are 0, bits 29 to 21 are `0_0000_0001` (1), bits 20 to 12 are `0_0000_0001` (1), and the low 12 bits are `0x234`. So the indices are **256, 0, 1, 1**, offset **0x234**. Bits 63 to 48 are all ones, which copy bit 47: canonical.

**Walk it.** Say CR3 holds `0x1000`.

| Level | Table at | Index | Entry read | Meaning |
|-------|----------|-------|------------|---------|
| 4 (PML4) | `0x1000` | 256 | `0x2003` | present, writable, next table at `0x2000` |
| 3 (PDPT) | `0x2000` | 0 | `0x3003` | next table at `0x3000` |
| 2 (PD) | `0x3000` | 1 | `0x4003` | next table at `0x4000` |
| 1 (PT) | `0x4000` | 1 | `0x5003` | the frame is `0x5000` |

The result is `0x5000 | 0x234 = 0x5234`. Four memory reads per translation, which is why the TLB exists.

The low bits of each entry (`3` = present + writable) are flags, so you mask them off with `0x000FFFFFFFFFF000` before treating the rest as an address. Forgetting that mask is the most common bug: `0x2003` is not a table address.

## Why 9 bits and 512 entries

An entry is 8 bytes. A table is one 4096-byte frame. So a table holds 4096 / 8 = **512 entries**, and 512 is 2^9, so each level consumes exactly 9 bits of the address. Four levels use 36 bits, and the 12-bit offset makes 48. That is why the address space is 256 TiB (2^48). Newer CPUs add a fifth level for 57 bits, which is also why bits above the top bit are "sign extension": hardware designers reserved the right to use those bits later without breaking old software that kept them as copies of the top bit.

## Reading the real tables

The code in this lesson treats a physical address as a pointer. That works here only because the boot stub identity-maps memory. A kernel that maps all RAM at some high offset uses `phys_to_virt(addr)` (add the offset) in the same place. If you ever see a crash while *reading a page table*, you probably used a physical address in a world where it isn't mapped.

## Debugging checklist

- **Everything returns `TRANSLATE_FAULT`:** you probably used `cr3` unmasked, or `pt_index` has the wrong shift. Level 1 shifts by 12, level 2 by 21, level 3 by 30, level 4 by 39.
- **Off by the page offset:** the final result needs `(entry & PTE_ADDR_MASK) | (virt & 0xFFF)`. For huge pages the offset is 21 or 30 bits wide, not 12.
- **Huge pages come out wrong:** check `PTE_HUGE` only at levels 3 and 2. At level 1 the same bit position means something else (PAT).
- **Permissions look too generous:** you took the leaf's flags only. Writable and user are the **AND** over all four levels; NX is the **OR**.
- **A crash while walking:** an entry's address bits point at memory that isn't mapped. In the tests that means a bug in how you advance `table` from one level to the next.

## Your turn

Implement `pt_index` and `translate`. The tests build their own tables in memory, and then check your walk against the machine's *real* boot page tables, the ones the CPU is using right now.
