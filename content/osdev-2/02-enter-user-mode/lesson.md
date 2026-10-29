---
title: Dropping to Ring 3 with iretq
summary: Build an iretq frame, jump into user mode, and get back to the kernel through a DPL 3 gate. Watch the CPU refuse privileged instructions and bad addresses from ring 3.
minutes: 70
difficulty: hard
tags: cpu, privilege, interrupts, assembly
id: osdev-2/enter-user-mode
mode: harness
---
The [last lesson](/learn/osdev-2/user-segments) built a GDT with ring 3 segments and a TSS. Now you use them. There is a surprise: the CPU has **no instruction that means "go to ring 3"**. Privilege only goes down by *returning* from something. So the kernel pretends there was an interrupt, builds the stack frame the CPU would have pushed if user code had been interrupted, and executes `iretq` on it.

## The iretq frame

When an interrupt arrives while the CPU is in ring 3, it pushes five 8-byte values on the kernel stack. `iretq` pops the same five. Lowest address first:

```
 low    [ rip     ]   where to resume
        [ cs      ]   0x23 = user code, RPL 3
        [ rflags  ]
        [ rsp     ]   the user's stack pointer
 high   [ ss      ]   0x1B = user data, RPL 3
```

`iretq` loads all five. The low two bits of the new `cs` say which ring to enter, so `0x23` means ring 3, and since the privilege level changes, the CPU also loads `rsp` and `ss` from the frame. A frame is just five numbers you can make up, which is what makes `iretq` the standard way to start a process.

`rflags` is `0x002`: bit 1 is reserved and always reads 1, and everything else is clear. In particular **IF (interrupts enabled) is clear**. The 8259 PIC has not been programmed in this lesson, so a timer tick would arrive on vector 8 (double fault). Preemption comes two lessons later. The IOPL bits are also zero, which is why ring 3 cannot use `in` and `out`.

## The way back: a gate with DPL 3

The ring 3 program has to be able to call the kernel *somehow*. The classic doorway is a software interrupt: `int 47` looks up gate 47 in the IDT. But the CPU checks the gate's **DPL** against the caller's CPL: with a ring 0 gate (type byte `0x8E`), `int 47` from ring 3 raises #GP instead. A gate that user code may use has DPL 3 (bits 5-6 of the type byte): `0xEE` instead of `0x8E`. Every other gate stays at DPL 0, which is exactly the protection you want: user code can reach only the entry points you chose.

When the interrupt arrives the CPU switches to the **`rsp0` stack from the TSS**, pushes the same five values (this time with the user's `ss`, `rsp`, `cs` and so on), and the hidden stub from Part 1 calls your hook `bm_exception_hook(vector, error_code, rip)`.

## Starting and ending a program

We use the same idea as your context switch. `enter_user(frame)` is a function that **seems to return much later**:

1. Push the callee-saved registers on the kernel stack and store `rsp` in `saved_kernel_sp`.
2. Point `rsp` at the iretq frame and `iretq`. The CPU is now in ring 3.

When the program traps (for any reason), the hook decides it is finished and calls `leave_user(value)`:

1. Load `rsp` from `saved_kernel_sp`. That throws away the interrupt frame we were running on; nothing needs it any more.
2. Pop the callee-saved registers, put `value` in `rax` and `ret`.

To the code that called `enter_user`, that is a normal return with `value` as the result. We use the *vector number* as the result, so the kernel can tell why the program stopped: 47 means "I'm done", 13 means a general protection fault, 14 a page fault.

## The page tables must allow it

A ring 3 access also needs a **U bit** in *every* level of the page-table walk (bit 2 of the PML4, PDPT, PD and PT entries). The boot stub maps memory for ring 0 only. The starter's `make_low_memory_user` sets the U bit on everything in the first 2 MiB, where your kernel and the test programs live, so ring 3 code can run there. That is deliberately too generous: it lets user code read and write the kernel! Two lessons from now, each process gets its own page tables and this goes away.

## Your turn

Implement the pieces listed in the task. The tests run small programs in ring 3: one reads its own `cs`, one tries `hlt`, one tries `in`, one dereferences address 8, one clobbers every callee-saved register, and each must come back to your kernel with the right result.
