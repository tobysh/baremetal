---
title: Physical Memory: The Frame Allocator
summary: Read the memory map the bootloader hands you, then track which 4 KiB frames of RAM are free with a bitmap, and allocate and free them.
minutes: 55
difficulty: medium
tags: memory, allocator
id: osdev/frame-allocator
mode: harness
---
Before a kernel can give memory to anything (page tables, stacks, a heap), it has to know what memory it *has*. RAM isn't one tidy block. Parts hold the BIOS, parts belong to devices, parts hold the kernel itself. Handing out the wrong bytes means corrupting something critical.

## What the bootloader tells you

QEMU loaded your kernel with the **Multiboot** protocol, which means that when `kmain` starts, a structure describing the machine is in memory and its address is in the variable `bm_multiboot_info`. The part we need is the **memory map**: a list of regions, each marked usable or reserved.

The info structure begins like this (all fields 32-bit, little endian):

| Offset | Field |
|--------|-------|
| 0 | `flags`: bit 6 is set when the memory map is present |
| 44 | `mmap_length`: size of the map in bytes |
| 48 | `mmap_addr`: address of the first entry |

Each entry in the map looks like this:

```
offset 0   u32  size      bytes in the rest of this entry (normally 20)
offset 4   u64  base      where the region starts
offset 12  u64  length    how many bytes
offset 20  u32  type      1 = usable RAM, anything else = reserved
```

The next entry begins `size + 4` bytes after this one. Always advance by the `size` field, not a fixed 24: the format allows entries to grow.

On a 64 MiB QEMU machine you'll see roughly: usable RAM from 0 to just under 640 KiB, a reserved hole for the BIOS and video memory, usable RAM from 1 MiB upwards, and a few reserved bits at the very top.

## Frames and a bitmap

Memory is managed in **frames** of 4096 bytes, aligned at multiples of 4096. Frame `n` starts at address `n * 4096`.

The simplest way to track them is a **bitmap**: one bit per frame, 1 for used and 0 for free. A 64 MiB machine has 16,384 frames, so the bitmap is only 2 KiB. To find a free frame, scan for a zero bit.

The allocator starts with *everything marked used*, then **frees only what the memory map says is usable**. That way any region the map forgot to mention is automatically safe.

Two rules about region edges:

- A frame is only usable if it lies **entirely** inside a usable region. A region from `0x1800` of length `0x3000` covers bytes `0x1800` to `0x4800`, so the frames at `0x2000` and `0x3000` are fully inside, while the ones at `0x1000` and `0x4000` are not.
- When *reserving*, round the other way: every frame the range even partly touches becomes used.

## What not to hand out

Even though the map says RAM is free, you must never allocate:

- **The first megabyte.** It holds the BIOS data area, video memory, and (importantly) frame 0, where a null pointer points.
- **Your own kernel image**, from 1 MiB up to the linker symbol `_kernel_end`. The bitmap itself, page tables and stack are in there too.

`pmm_setup` ties it together. The tests then allocate real frames and write to them, so a mistake corrupts something real.

## A worked example

Take a toy machine with 16 frames (64 KiB) and this memory map:

| Base | Length | Type |
|------|--------|------|
| `0x0000` | `0x8000` | usable |
| `0x8000` | `0x1800` | reserved |
| `0x9800` | `0x6800` | usable |

Start with every bit set: the bitmap bytes are `FF FF`. Now add the regions.

- Region 1 covers `0x0000` to `0x8000`. Frames 0 to 7 lie entirely inside it, so bits 0 to 7 are cleared: `00 FF`.
- Region 2 is reserved, so nothing changes.
- Region 3 covers `0x9800` to `0x10000`. Round the start **up**: `0x9800` is inside frame 9 (`0x9000` to `0xA000`), so the first frame that is entirely inside is frame 10 (`0xA000`). Frames 10 to 15 are freed: `00 03`. Frame 8 and frame 9 stay used. Frame 9 is half reserved memory, and a frame is either wholly yours or not yours.

Reading the bitmap: byte 1 is `0x03`, so bits 0 and 1 of byte 1 are set. Those are frames 8 and 9. Frame `i` lives in byte `i / 8`, bit `i % 8`, and that is the only index arithmetic in the whole allocator.

Reserving the first MiB then sets the low bits again, `pmm_alloc` hands out the lowest zero bit, and the free count moves with every bit that actually changes.

## Why the memory map looks like it does

The odd hole below 1 MiB is a fossil. The original IBM PC could address 1 MiB. The top 384 KiB of that space was given to the BIOS ROM, video memory and expansion cards, so RAM was split into the "conventional" 640 KiB and whatever was installed above 1 MiB. Every PC since has kept that layout for compatibility. The firmware reports it through the BIOS call `INT 15h, AX=E820h` ("E820"), and the bootloader (here QEMU's Multiboot loader) copies the answer into the info structure you parse. The `type` values 1 (usable), 2 (reserved), 3 (ACPI reclaimable) and 4 (ACPI NVS) come from that BIOS call too.

## Other designs

A bitmap costs 1 bit per frame and always scans from the start, so `pmm_alloc` is slow when the front of memory is full. Other allocators trade differently. A **free list** stores a "next free" pointer inside each free frame, so allocation is constant time but you need the memory to be mapped to follow the pointers. A **buddy allocator** keeps lists of power-of-two blocks and splits and merges them, which finds contiguous runs fast. Linux uses buddies; many hobby kernels start with a bitmap because it is the easiest to get right and to debug. Correct first, fast later.

## Debugging checklist

- **Free count slightly too high or low:** you changed `nfree` without checking whether the bit actually flipped. Adding a region that overlaps a reserved one, or freeing a frame twice, must not move the count.
- **Frame 0 comes back from `pmm_alloc`:** you didn't reserve the first MiB. A null pointer is address 0, and a page-fault is the best result you can hope for.
- **Writes to an allocated frame corrupt the kernel:** the reservation of `0x100000` to `_kernel_end` is missing or rounds the wrong way. Reservations round the range *outward*.
- **A frame just beyond a region's edge is free:** the start rounds up and the end rounds down for `pmm_add_region`. Doing the opposite hands out memory that belongs to something else.
- **Crash with a page fault inside `mmap_parse`:** you advanced by 24 bytes instead of `size + 4`, or read the 64-bit fields at the wrong offsets (base at +4, length at +12, type at +20).

## Your turn

Implement the parser and the allocator. Frame addresses are `uint64_t`. `PMM_NONE` is returned when memory runs out.
