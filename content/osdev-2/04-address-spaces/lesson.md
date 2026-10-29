---
title: One Address Space per Process
summary: Create isolated address spaces, map and copy pages, switch CR3, and reclaim frames safely.
minutes: 65
difficulty: hard
tags: memory, paging, processes, cr3
id: osdev-2/address-spaces
mode: harness
---
Until now every piece of code you ran saw the same memory. Last lesson's ring 3 programs could even read and write the kernel, because the U bit was set on the whole low 2 MiB. That is no isolation at all. The tool for real isolation is the one you met in [Part 1](/learn/osdev/page-tables): **page tables**. Give every process *its own top-level table*, load it into `cr3` when that process runs, and the same virtual address can mean different memory in different processes, or nothing at all.

## A PML4 per process

The top level, the **PML4**, has 512 entries; each covers 512 GiB of virtual address space. An address space is therefore just a 4 KiB frame holding 512 entries. A new, empty process starts with *all entries zero* and slowly grows its own tree below it.

There is one catch. When the CPU takes an interrupt or a system call, **it keeps running with the process's `cr3`**. The kernel's code, stacks and data must therefore be mapped in every address space too, or the first interrupt would jump into unmapped memory and triple-fault. The cheap way to do that is sharing: copy the kernel's top-level entry into every new PML4. All the address spaces then point at the *same* lower-level tables for the kernel part, so a change there shows up everywhere.

In our identity-mapped kernel everything below 512 GiB lives under PML4 entry 0. We use that entry for the kernel and give user processes everything else:

```
PML4 entry 0         kernel (shared, the same PDPT in every space; U bit clear)
PML4 entries 1..255  user space, starting at USER_BASE = 0x8000000000 (512 GiB)
```

Real kernels place the kernel in the *high* half instead (entries 256 to 511), but the idea is identical.

## What the starter gives you

A small pool of 64 page frames (`frame_alloc` returns a zeroed frame, `frame_free` gives one back and counts any bad free in `bad_frees`), the page-table flags, `map_page(pml4, virt, phys, flags)` that builds missing levels with the U bit set, and `leaf_pte(pml4, virt)` to find a page's final entry. You write the process-level operations:

- **`aspace_create`**: allocate a PML4 and copy entry 0 from `kernel_pml4`. Nothing else.
- **`aspace_alloc_page`**: take a frame and map it at a user address, with the U bit and optionally writable. It refuses addresses outside the user half and gives the frame back if the mapping fails.
- **`aspace_switch`**: load `cr3`. Writing `cr3` also flushes all non-global TLB entries, which is why switching address spaces is a slow operation.
- **`aspace_copy_to`**: copy bytes into another address space *without switching to it*, the way a program loader fills in a new process. The kernel can do this because all physical frames are identity-mapped, so it walks the target's tables in software, finds the frame behind each virtual page and writes there directly. A copy may cross page boundaries, and neighbouring virtual pages are usually *not* neighbouring in physical memory.
- **`aspace_destroy`**: free everything a process owns: its data frames, its page-table frames at every level, and its PML4. **Never free the shared kernel tables**, which belong to every process, and never use a PML4 that is currently loaded: if it is the current space, switch back to the kernel's first.

## The trap in `aspace_destroy`

A recursive walk over the tree is natural: for each present entry, free what it points to, then free the table itself. If you start the walk at entry 0, you free the kernel's tables and take the whole machine down with you. Start at entry 1. Lower-level tables belong to the process, and the walk's leaf level holds *data* frames, which also belong to it in this lesson.

## Seeing isolation

After `aspace_switch(B)`, an address that only A had mapped is simply not present. The CPU raises a **page fault** (vector 14) with the address in `cr2`. In the tests, a hook handles that fault by mapping a fresh page, which is demand paging in miniature. Reading with the supervisor bit set from kernel mode makes no difference: the *page tables* decide.
