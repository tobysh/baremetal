---
title: Physical Frame Allocation
summary: Track available physical page frames with a bitmap and reserve ranges safely.
minutes: 45
difficulty: medium
tags: kernel, systems
id: osdev-2/physical-frames
mode: harness
---
Every other memory structure in a kernel sits on top of one primitive: "give me a 4 KiB page of physical RAM that nobody else is using". Page tables need frames for their tables, [address spaces](/learn/osdev-2/address-spaces) need frames for each process's data, the heap in the next lesson needs frames to carve up, and every kernel stack is a frame or two. The **frame allocator** is that primitive. In [Part 1](/learn/osdev/frame-allocator) you built one over the whole machine from the bootloader's memory map. This lesson strips it down to the core so you can get the edge cases exactly right: a pool of 64 frames, one 64-bit word of bookkeeping, and three operations that must never corrupt it.

## One bit per frame

A **frame** is a 4096-byte, 4096-aligned block of physical memory. Our pool has 64 of them starting at `BASE = 0x100000` (1 MiB), so frame `i` lives at:

```text
address(i) = BASE + i * 4096          index(addr) = (addr - BASE) / 4096

index   0         1         2         3              63
      +---------+---------+---------+---------+   +---------+
      |0x100000 |0x101000 |0x102000 |0x103000 |...|0x13F000 |
      +---------+---------+---------+---------+   +---------+
bit      0         1         2         3              63      of `used`
```

Sixty-four frames fit exactly in one `uint64_t`. Bit `i` of `used` is **1 when frame `i` is unavailable** and 0 when it is free. "Unavailable" covers two different reasons with the same bit: the frame was handed out by `frame_take`, or it was *reserved* because something else already lives there (the kernel image, a DMA buffer, a firmware table). The allocator does not care which; it only has to never hand the frame out twice.

The three bit operations you need, for a valid index `i`:

```c
used & (1ULL << i)     /* test:  non-zero if frame i is unavailable */
used |= 1ULL << i;     /* set:   mark it unavailable                */
used &= ~(1ULL << i);  /* clear: mark it free                       */
```

Note the `ULL`. A plain `1 << 40` is an `int` shift by more than its width, which is undefined behaviour; on x86 it usually produces `1 << 8` and silently marks the wrong frame. Shifting by 64 or more is undefined even for `uint64_t`, so the index must be validated **before** the shift.

## Why zero is the failure value

`frame_take` returns a physical address, so it needs a value that can never be a real frame to signal "out of memory". Zero is the natural choice because address 0 is never handed out: the pool starts at 1 MiB, and in a real kernel page 0 is deliberately left unmapped so that null pointers fault. The caller writes `if (!frame) return -ENOMEM;` and the convention costs nothing.

## Reserving a range

`frame_reserve(first, count)` marks the half-open index range `[first, first + count)`. "Half-open" means `first` is included and `first + count` is not, so `count` is the number of frames and an empty range has `count == 0`. Two properties matter.

**It clamps to the pool.** The caller may describe a region that overlaps the end of the pool, or lies entirely past it. Only the intersection with frames 0 to 63 is marked; anything beyond is ignored, not an error.

**It is idempotent.** Reserving a frame that is already reserved or taken leaves it set. Setting a bit with `|=` is naturally idempotent, which is one reason a bitmap is easy to get right.

The trap is unsigned overflow. With `unsigned` arguments, `first + count` can wrap around:

```text
first = 10, count = 0xFFFFFFFF
first + count = 0x100000009  -> truncated to 32 bits -> 9

for (i = first; i < first + count; i++)   /* i < 9 is already false: reserves nothing! */
```

A caller asking to reserve "everything from frame 10 onwards" gets nothing reserved, and the allocator later hands out memory that belongs to someone else. The fix is to never compute `first + count` until you know it cannot wrap. Check `first` against the pool first, then compare `count` with the room that is left:

```c
if (first >= 64) return;          /* nothing of the range is in the pool */
if (count > 64 - first)           /* 64 - first cannot underflow now */
    count = 64 - first;
/* first + count <= 64 from here on */
```

This "subtract instead of add" shape comes back in the RAM disk and file lessons; it is the standard way to bounds-check with unsigned numbers.

## Taking a frame

`frame_take` scans bits 0 to 63 and stops at the first clear one. It must **set the bit before returning**; otherwise the next call returns the same frame and two owners write over each other. Lowest-first is a policy choice, but a useful one: it makes the allocator deterministic, which is what you want when debugging (and what the tests rely on).

If every bit is set, the pool is exhausted and the function returns 0 without changing anything.

## Releasing a frame

`frame_release(addr)` is where a kernel meets other people's bugs, so it validates everything before touching the bitmap. Work through the address in this order:

```text
addr < BASE                    -> -1   below the pool
addr >= BASE + 64 * 4096       -> -1   above the pool
(addr - BASE) % 4096 != 0      -> -1   points into the middle of a frame
bit index is already clear     -> -1   double free (or never allocated)
otherwise                      ->  0   clear the bit
```

The order is not cosmetic. Computing the index of an address below `BASE` underflows to an enormous number, and shifting by it is undefined behaviour. Range checks come first, then alignment, then the index.

The double-free check is the most valuable line in the function. A double free that silently "succeeds" means the frame is now free while its first owner still uses it; the next `frame_take` gives it to a second owner and the corruption surfaces much later, somewhere unrelated. Returning -1 lets the caller panic at the actual bug. In this lesson releasing a *reserved* frame is allowed and returns 0: reservation and allocation share a bit, and a kernel that reserved boot-time memory may well hand it back later.

## A worked example

```text
frame_init()                used = ...0000 0000
frame_reserve(0, 2)         used = ...0000 0011     frames 0,1 reserved
frame_take()  -> 0x102000   used = ...0000 0111     lowest clear bit was 2
frame_take()  -> 0x103000   used = ...0000 1111
frame_release(0x102000)     used = ...0000 1011  -> 0
frame_release(0x102000)                          -> -1  (bit already clear)
frame_release(0x102001)                          -> -1  (unaligned)
frame_take()  -> 0x102000   the hole is reused first
```

## Beyond this exercise

Real allocators add a lock (two CPUs taking the same clear bit is exactly the race the [spinlocks](/learn/osdev-2/spinlocks) lesson handles), a hint of where the last free frame was found so the scan does not always start at 0, and an array of words instead of one. The bit arithmetic stays the same: word `i / 64`, bit `i % 64`.
