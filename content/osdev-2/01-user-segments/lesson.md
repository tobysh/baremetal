---
title: Ring 3 Ready: User Segments and the TSS
summary: Build a GDT with user code and data segments and a 16-byte TSS descriptor, load the task register, and check the result against what the CPU itself reports.
minutes: 50
difficulty: medium
tags: cpu, privilege, gdt, tss
id: osdev-2/user-segments
mode: harness
---
So far everything you wrote ran in **ring 0**, the most privileged level: it can run any instruction, touch any port and map any memory. A real operating system runs its *users'* programs in **ring 3**, where the CPU refuses to run `hlt`, `in`, `out`, `lgdt` and friends, and where memory is only reachable if the page tables mark it as user memory. Everything else in this course (system calls, processes, isolation) rests on that boundary.

To cross the boundary the CPU needs two things from you, and both live in the GDT.

## Segments still matter in 64-bit mode

Base and limit are ignored in long mode, but the **privilege bits** of the segment registers are not. A running program has a **CPL** (current privilege level) which is simply the low two bits of `cs`. A GDT entry carries a **DPL** (descriptor privilege level) in bits 5 and 6 of its access byte. Your Part 1 kernel needed only DPL 0 entries (`0x9A` code, `0x92` data). For ring 3 you add the same two entries with the DPL bits set, which turns the access bytes into `0xFA` (code) and `0xF2` (data).

A **selector** is an index into the GDT times 8, plus a 2-bit **RPL** (requested privilege level) in its low bits. The selector for GDT slot 3 used by ring 3 is `3 * 8 | 3 = 0x1B`.

The order of the entries is not arbitrary. Two lessons from now you will use the `syscall`/`sysret` instructions, and `sysret` computes the user selectors from one number: user data must sit exactly one slot *before* user code. So the table is:

```
0x00  null
0x08  kernel code   access 0x9A   flags 0xA
0x10  kernel data   access 0x92   flags 0xC
0x18  user data     access 0xF2   flags 0xC     (selector 0x1B with RPL 3)
0x20  user code     access 0xFA   flags 0xA     (selector 0x23 with RPL 3)
0x28  TSS           16 bytes: it takes slots 0x28 and 0x30
```

The flags nibble `0xA` is granularity plus the **L** bit (this is a 64-bit code segment); `0xC` is granularity plus the 32-bit default size bit that data segments carry.

## The TSS

When a ring 3 program is interrupted (a timer tick, a page fault, an `int` instruction), the CPU must switch to a **kernel stack**: it cannot trust the user's `rsp`, which might be garbage or even point at kernel memory. Where does it find a safe stack? In the **Task State Segment**. In the old days the TSS stored whole task contexts. In 64-bit mode it only holds a few fields:

```c
typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0, rsp1, rsp2;   // stack pointers for rings 0, 1, 2
    uint64_t reserved1;
    uint64_t ist[7];             // interrupt stack table
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;        // offset of the I/O permission bitmap
} Tss;                           // 104 bytes
```

Only `rsp0` matters here: on any switch from ring 3 to ring 0 the CPU loads `rsp` from it. If it is wrong, the first interrupt from user mode triple-faults.

`iopb_offset` says where the **I/O permission bitmap** starts, which would let selected ports be used from ring 3. If it is *at or past the TSS limit* there is no bitmap, and every `in`/`out` from ring 3 faults. That is what we want, so set it to `sizeof(Tss)`.

## The TSS descriptor is 16 bytes

Like an IDT gate, a system descriptor in long mode is twice the usual size, because the base is a full 64-bit address:

```
bytes 0-1   limit bits 0-15           (sizeof(Tss) - 1)
bytes 2-3   base  bits 0-15
byte  4     base  bits 16-23
byte  5     access: 0x89 = present, DPL 0, type 9 "64-bit TSS (available)"
byte  6     limit bits 16-19 in the low nibble, flags in the high nibble (zero here)
byte  7     base  bits 24-31
bytes 8-11  base  bits 32-63
bytes 12-15 reserved, zero
```

Once the table is loaded you tell the CPU which descriptor is the TSS with the `ltr` instruction, giving it the *selector* (`0x28`). The CPU then marks the descriptor **busy** by rewriting its type from `0x9` to `0xB`, i.e. access byte `0x89` becomes `0x8B`, *in your table in memory*. That is a handy proof that the CPU really used it.

## Loading a new GDT

`lgdt` only sets the GDT register. The segment registers still hold the *old* cached descriptors until reloaded. Data registers can simply be loaded with `mov`, but `cs` can only be changed by a far jump or return. The starter's `gdt_flush` does all of that with a `lretq`; you provide the table and its pointer.

## Your turn

The structures, `gdt_entry`, `gdt_flush` and `load_task_register` are in the starter. You write the TSS pieces and `gdt_setup`.
