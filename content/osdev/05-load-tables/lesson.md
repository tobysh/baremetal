---
title: Loading Tables and Catching Exceptions
summary: Build and load your own GDT and IDT with lgdt and lidt, reload the segment registers, and catch your first real CPU exceptions in a C handler.
minutes: 50
difficulty: medium
tags: cpu, descriptors, interrupts
id: osdev/load-tables
mode: harness
---
Last lesson you built single entries. Now you build whole tables, hand them to the CPU, and see what happens when an exception fires. The boot stub already set up a temporary GDT and IDT so your code could start. A real kernel replaces them with its own as one of its first jobs.

## The GDT

You need three entries:

| Selector | Entry | Meaning |
|----------|-------|---------|
| `0x00` | null | The CPU requires entry 0 to be all zeros |
| `0x08` | kernel code | access `0x9A`, flags `0xA` (the **L** bit: 64-bit code) |
| `0x10` | kernel data | access `0x92`, flags `0xC` |

A **selector** is an index into the table times 8 (plus privilege bits). That is why code is `0x08` and data is `0x10`.

One surprise: when the CPU loads a segment register it sets bit 0 of the descriptor's access byte (the **accessed** bit) *in the table itself*. If you read your table back afterwards you may see `0x93` where you wrote `0x92`. That is the hardware writing to your memory, and it is why the GDT must live in writable memory. The tests ignore that bit.

Getting the code flags right matters. Flags `0xC` (the 32-bit **D** bit) on a code segment makes the CPU run in *compatibility mode*, a 32-bit sub-mode of long mode. The very next instruction then decodes as garbage.

Loading the table takes two steps:

1. `lgdt` loads the table pointer. It does **not** change the segment registers: they keep their old cached values.
2. Reload them. Data segments (`ds`, `es`, `ss`, `fs`, `gs`) can be loaded with `mov`. `cs` can't, so the usual trick is a **far return**: push the new selector, push the address to continue at, then `lretq`.

### Why the segment registers need reloading

Each segment register has a visible 16-bit selector and a hidden **cache** holding the descriptor it pointed at when you loaded it. The CPU uses the cache, not the table, while it runs. So after `lgdt` replaces the table, the registers still hold the *old* descriptors until you load them again. Reloading means "read the new table and refresh the cache". If you skip it, nothing seems to break at first, then something that uses the new table (a later segment load, or an interrupt that switches `cs`) behaves strangely.

`cs` is special because it decides where code is fetched from, and the CPU will not let you `mov` into it. A far return does the job because `lretq` pops two values: the new instruction pointer and then the new `cs`. So you push the selector, then push the address of the next instruction, and execute `lretq`:

```asm
push 0x08              # new cs (kernel code selector)
lea  rax, [rip + 1f]   # address of the label just below
push rax               # new rip
lretq                  # pops rip, then cs: jumps to 1: with the new cs loaded
1:
```

That is the same trick the starter uses. A common source of confusion is the order of the two pushes: whatever is on top of the stack becomes `rip`, and the next value becomes `cs`.

The starter gives you `load_gdt()` and `load_idt()` with this assembly already written. Read them: they're short and they're what real kernels do.

## The IDT

Vectors 0 to 31 are CPU **exceptions**: 0 is divide error, 3 is a breakpoint (`int3`), 6 is an invalid opcode, 13 is a general protection fault, 14 is a page fault. Each needs a gate in the IDT pointing at code that handles it.

Writing a handler directly in C isn't possible, because the CPU pushes its own frame and a handler must save every register and end with `iretq`, not `ret`. Real kernels use a small assembly stub per vector. The boot stub provides them: `bm_isr_table[i]` is the address of the stub for vector `i`. Each stub saves the registers and calls the function pointed to by `bm_exception_hook`, then restores everything and returns with `iretq`.

So your job is:

1. Fill the IDT: gates for vectors 0 to 31, each using `bm_isr_table[i]`, selector `0x08` and type `0x8E`.
2. Load it with `load_idt()`.
3. Point `bm_exception_hook` at your C handler.

### What the CPU does when an exception fires

At the moment of an exception the CPU is in the middle of your code. To resume it later, it has to save enough state to return. In 64-bit mode it pushes a fixed **interrupt frame** on the stack and then jumps to your gate's handler:

```
higher addresses
+------------------+
| SS               |
| RSP              |  where the stack was
| RFLAGS           |  flags, including whether interrupts were on
| CS               |
| RIP              |  the instruction that was running (or the next one)
| error code       |  only for some vectors (see below)
+------------------+  <- rsp on entry to the handler
lower addresses
```

The CPU pushes the error code only for a few vectors: 8 (double fault), 10, 11, 12, 13 (general protection), 14 (page fault), 17, and a few more. For the others there is no error code on the stack. The frame then has a different size, which would make a shared handler awkward. That is why the boot stub's per-vector code pushes a dummy zero for vectors without one, so every handler sees the same layout. This is also why the handler's `error_code` parameter is "zero when the CPU pushes none".

The error code means different things for each vector. For a general protection fault it identifies the selector involved. For a page fault it is a set of flag bits (was it a read or a write? was the page present?). Page faults also leave the address that failed in a control register, `cr2`; you will read it in the paging lesson.

The last step in the handler path is `iretq`. It is the reverse of the CPU's push: it pops `rip`, `cs`, `rflags`, `rsp` and `ss`, and resumes where it left off. It cannot be written in C, which is why the assembly stub exists. For a **fault** such as a page fault, the saved `rip` points at the instruction that caused it, so after you fix the problem the CPU retries it. For a **trap** such as `int3`, `rip` points at the instruction *after* it, so execution simply continues.

### Interrupt gates and the interrupt flag

When the CPU enters through an *interrupt gate* (type `0x8E`) it clears the **IF** flag, so no further maskable interrupts are delivered while your handler runs. `iretq` restores the old `rflags`, and therefore the old IF, on the way out. A trap gate (`0x8F`) leaves IF alone. For exceptions you want the interrupt gate: you do not want a timer tick interrupting a handler that is reporting a crash.

### Triple faults

What happens if the CPU has an exception, but the IDT has no valid gate for it? It raises a fault about the table itself instead: a **general protection fault** if the vector lies beyond the IDT's limit, or a **segment not present** fault (vector 11) if the gate is not marked present. If delivering *that* fails too, it raises a **double fault** (vector 8). If even the double fault handler cannot be called, the CPU gives up and performs a **triple fault**, which resets the machine. On real hardware the screen just goes black and the computer reboots. In the lab you see the machine restart with no message. The chain is why a wrong IDT is so hard to debug: the first error is hidden behind two more.

When you see this, check things in this order: is the `idt` pointer's limit `32 * 16 - 1`? Is every gate's selector `0x08`? Is the type byte `0x8E`, so the gate is present? Is the GDT loaded before the IDT, so selector `0x08` refers to a real code segment? A missing present bit or a wrong selector is almost always the answer.

## Your handler

The handler receives the vector number, the error code (zero when the CPU pushes none) and the address of the instruction that was interrupted. Yours records them so the tests can see what happened. For `int3` and the software interrupts the tests use, execution simply continues after the handler returns.

## Your turn

Implement `gdt_install`, `idt_install`, `exception_handler` and `exceptions_init`. If something is wrong, the machine will probably triple-fault: a fault with no working handler. Compare your tables with the lesson.
