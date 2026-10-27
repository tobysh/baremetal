---
title: Mapping and Unmapping Pages
summary: Edit the live page tables to map a new virtual page, allocate missing tables on demand, flush the TLB on unmap, and catch the resulting page fault.
minutes: 55
difficulty: hard
tags: memory, paging
id: osdev/map-pages
mode: harness
---
[Last lesson](/learn/osdev/page-tables) you read page tables. Now you **change** them, while the CPU is running on them. This is what `mmap`, `malloc` and loading a program all come down to: pick a virtual address, pick a physical frame, make the tables say so.

## Mapping one page

To map `virt` to `phys`, walk from the top just as `translate` does, but wherever an entry is **not present**, build the missing table:

1. Take a free frame (`alloc_frame()` in the starter returns a zeroed one, or 0 when memory is gone).
2. Store its physical address in the entry with the present, writable and user bits: `frame | PTE_PRESENT | PTE_WRITABLE | PTE_USER`.
3. Continue the walk into that new table.

At level 1, store `phys | flags | PTE_PRESENT` in the final entry. Intermediate entries are made permissive on purpose. A mapping is only as permissive as the *most restrictive level* (last lesson), so the restriction is placed in the leaf entry.

Details that matter:

- Both addresses must be page-aligned, and `virt` must be canonical. Otherwise return -1.
- If the walk reaches a **huge page** (PS bit) at level 3 or 2, there is no lower table to put an entry in. Return -1.
- If the leaf is already present, return -1. Silent remapping hides bugs.
- If you map a **user** page, every existing entry on the path must also have the user bit, or user mode still can't reach it. The boot tables have no user bits at all, so you need to set it on existing entries too.

## Unmapping and the TLB

Walking four levels of tables for every memory access would be slow, so the CPU caches recent translations in the **TLB** (translation lookaside buffer). The CPU does not watch the page tables. If you clear an entry and the translation is cached, the CPU keeps using the stale one and the unmapped page *still works*, until the entry happens to be evicted.

After changing or removing a mapping, tell the CPU to forget that address:

```c
__asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
```

Changing CR3 flushes everything, but `invlpg` flushes one page, which is far cheaper. (The starter gives you `tlb_invalidate`.) Newly *adding* a mapping needs no flush: x86 never caches "not present".

## Page faults

Touching an unmapped page raises exception **14**. The CPU pushes an error code (bit 0: 1 if the page was present but access was denied, 0 if it was not present; bit 1: the access was a write) and puts the faulting address in register **CR2**. A kernel reads CR2 and decides: kill the program, or map a page and return, so the access retries and succeeds. That's **demand paging**, the basis of lazy allocation and swapping. The tests do exactly that to your `unmap_page`.

## A worked example in the boot tables

The boot stub built these tables: `PML4[0]` points to a PDPT, `PDPT[0]` points to a PD, `PD[0]` points to a PT covering the first 2 MiB with 4 KiB pages (page 0 left absent), and `PD[1..511]` are 2 MiB huge pages. Everything above 1 GiB is absent.

Now call `map_page(cr3, 0x40000000, frame_a, PTE_WRITABLE)`. The address splits into indices **0, 1, 0, 0**:

1. Level 4: `PML4[0]` is present. Go on. (Nothing to allocate.)
2. Level 3: `PDPT[1]` is **absent**. Allocate a frame for a new PD, store `frame | P | W | U` in `PDPT[1]`, continue into it.
3. Level 2: `PD[0]` of that new table is absent. Allocate a frame for a new PT, store it, continue.
4. Level 1: `PT[0]` is absent, so write `frame_a | PTE_WRITABLE | PTE_PRESENT`.

Two frames were consumed, which is exactly what the test counts. A second call for `0x40001000` walks the same path (indices 0, 1, 0, **1**), finds every table present and only writes `PT[1]`: zero allocations.

## Why `invlpg`?

The TLB is a cache keyed by virtual address. When the CPU translates an address it stores the result and never rereads the tables for that page until the entry is evicted or flushed. The CPU does not snoop your stores to page-table memory, because doing so for every store would cost far more than it saves. So *you* flush.

- **Adding** a mapping where there was none needs no flush: the CPU never caches "not present", so the next access walks the tables and sees your new entry.
- **Removing or changing** a mapping needs `invlpg` for that page. Skip it and the old translation keeps working, which can leak memory you thought you'd unmapped.
- **Switching address spaces** (writing CR3) flushes all non-global entries at once.

On a multiprocessor system, `invlpg` only affects the CPU that ran it. Other CPUs must be told with an inter-processor interrupt, the famous **TLB shootdown**. This is one of the reasons unmapping memory is expensive on big machines.

## Reading a page fault

When the CPU raises exception 14 it gives you two pieces of information. The faulting address is in **CR2**. The error code on the stack has these bits:

| Bit | Value 0 | Value 1 |
|-----|---------|---------|
| 0 (P) | page was **not present** | protection violation on a present page |
| 1 (W) | the access was a read | the access was a write |
| 2 (U) | kernel mode | user mode |
| 4 (I) | data access | instruction fetch |

The test's handler does what a demand-paging kernel does: read CR2, map a page there, and return. `iretq` then re-executes the instruction that faulted, and this time it works.

## Debugging checklist

- **A triple fault right after your first `map_page`:** the new table frame wasn't zeroed, or you stored a *virtual* pointer into an entry instead of the physical address. (`alloc_frame` returns zeroed physical frames here.)
- **The mapping "works" but reads garbage:** you stored the flags but not the frame's address, or you lost the present bit.
- **A user mapping is not user-accessible:** `translate` shows the user bit missing, which means an *existing* intermediate entry has no `PTE_USER`. Set it on every level of the path.
- **Mapping succeeds twice at the same address:** you didn't check whether the leaf was already present.
- **Unmapped pages stay readable:** you forgot `tlb_invalidate`.

## Your turn

The starter provides `translate` from [last lesson](/learn/osdev/page-tables) (finished), `alloc_frame`, the counter `frames_used`, `tlb_invalidate` and `read_cr3`. Implement `map_page` and `unmap_page`. The tests touch memory through your new mappings, so a wrong entry gives a real page fault.
