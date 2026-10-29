---
title: Demand Paging: Handling Page Faults
summary: Treat a page fault as a request: reserve regions with permissions, then allocate and map a zeroed frame on first touch and let the instruction retry.
minutes: 70
difficulty: hard
tags: memory, paging, exceptions
id: osdev/lazy-paging
mode: harness
---
Until now a page fault meant "something is wrong". In a real kernel it is mostly a *request*. When a program asks for a gigabyte of memory, the kernel does not hand over a gigabyte of RAM. It records "this address range is allowed" and does nothing else. Physical memory is allocated one page at a time when the program **first touches** it, and the program never notices. This is **demand paging**, and the tool is the page-fault handler you will write.

## The life of a lazy page

```
program:   mov byte [0x80003000], 7      <- the page is not mapped
CPU:       #PF  (vector 14), CR2 = 0x80003000, error code = 0b010 (write, not present)
handler:   is 0x80003000 inside a region we promised?  yes, writable
           frame = frame_alloc(); zero it
           map_page(0x80003000 & ~0xFFF, frame, PRESENT | WRITABLE)
           return from the exception
CPU:       re-executes the SAME instruction (the saved RIP points at it)
program:   the store succeeds; it never knew
```

The important detail is that a **fault** (unlike a trap) saves the address of the instruction that failed, not the one after it. When the handler returns with `iretq`, the CPU simply tries again, and this time the page is there.

## Regions

The kernel needs to know which addresses are legal. Each **region** is a page-aligned range with a permission:

```c
typedef struct { uint64_t start, end; int writable; } Region;   // [start, end)
```

Only addresses inside a region get a page on demand; a fault anywhere else is a real bug (a wild pointer) and is refused.

## Reading the error code

The error code tells the handler *why* it faulted (you decoded it in the crash-report lesson):

| Condition | Verdict |
|---|---|
| bit 0 set (the page **is** present) | a **protection** fault: writing to a read-only page, say. Mapping something new cannot fix this. Refuse. |
| bit 4 set (instruction fetch) | our regions hold data only. Refuse. |
| bit 1 set (write) and the region is read-only | refuse |
| address inside no region | refuse |
| otherwise | allocate, zero, map, accept |

A read-only region still gets lazily mapped on a *read* (with the writable bit clear). A later write to that page then faults again, this time with bit 0 set, and is refused.

## Zero it first

A freshly allocated frame contains whatever the last user left there: maybe a password or a key. Handing it out without clearing it is a classic information leak. Every frame mapped on demand must be zeroed first, and programs rely on this ("memory from the OS starts as zero").

## Running out of memory

`frame_alloc` returns 0 when RAM is exhausted. The handler must then refuse the fault (return "not handled") rather than map physical address 0. What a real kernel does next (kill the program, free caches, swap) is a later problem.

## What is provided

The starter has a small bump `frame_alloc`, a `map_page(virt, phys, flags)` that walks the live page tables from `CR3` (creating missing tables, then executing `invlpg`), `read_cr2`, and `panic`. You are building the policy on top of them: which faults to accept, and the handler that wires it into the CPU.

## Debugging checklist

- **The kernel hangs after the first access:** the handler returned without mapping anything, so the CPU retries the same instruction and faults forever. A refused fault must `panic`, not return.
- **Memory is not zero:** you mapped the frame before clearing it, or cleared the wrong address. Clear through its physical address (RAM is identity-mapped here).
- **The first page works, the second faults forever:** you mapped the region's start instead of the faulting address rounded down to a page.
- **A read-only page can be written:** you mapped every page writable regardless of the region.
- **Overlapping regions are accepted:** compare `[start, end)` ranges properly: they overlap if `start < other.end && other.start < end`.

## Your turn

The tests touch real unmapped addresses, so the faults are real. They check the page contents, the permission bits in the live page tables, and the behaviour when memory runs out.
