---
title: The syscall Instruction
summary: Program EFER, STAR, LSTAR and SFMASK, write the assembly entry stub that swaps stacks and preserves registers, and return to ring 3 with sysretq.
minutes: 70
difficulty: hard
tags: cpu, syscalls, msr, assembly
id: osdev-2/syscall-instruction
mode: harness
---
In [Part 1](/learn/osdev/syscalls) you wrote the *middle* of a system call: the table, the error codes and the pointer checks. This lesson builds the doorway itself, the CPU instruction pair `syscall` and `sysret`, which replaced `int 0x80` because an interrupt gate does a lot of slow work (IDT lookup, privilege checks, pushing five values) that a system call does not need.

## What `syscall` does

When user code executes `syscall`, the CPU:

1. saves the address of the **next** instruction in `rcx` and `rflags` in `r11`;
2. clears the `rflags` bits named in the **SFMASK** register (we clear IF so no interrupt arrives while we are half-way through switching stacks);
3. loads `cs` and `ss` from the **STAR** register, and sets `rip` from **LSTAR**.

That is all. It does **not** switch the stack: `rsp` still points into the user's memory, which the kernel must never trust. Your entry stub has to load a kernel stack *before* it pushes anything.

`sysretq` does the reverse: `rip = rcx`, `rflags = r11`, and `cs`/`ss` are loaded from STAR for ring 3. It does not touch `rsp`, so the stub restores the user's stack pointer just before it.

## The four registers

These are **model-specific registers**, read with `rdmsr` and written with `wrmsr` (both take the MSR number in `ecx`, and the 64-bit value split across `edx:eax`). The starter has `rdmsr`/`wrmsr` helpers.

```
0xC0000080  EFER     bit 0 (SCE) enables syscall/sysret; without it the instruction is #UD.
                     Keep the other bits: long mode (LME/LMA) lives here too!
0xC0000081  STAR     bits 47:32 = kernel CS for syscall, so SS = that + 8
                     bits 63:48 = base for sysret: user SS = base + 8, user CS = base + 16
0xC0000082  LSTAR    the 64-bit address of your entry stub
0xC0000084  SFMASK   rflags bits cleared on entry (use 0x200, IF)
```

The GDT of the [previous lesson](/learn/osdev-2/user-segments) was laid out for exactly this. With the kernel code at `0x08` and kernel data at `0x10`, put `0x08` in bits 47:32. For `sysret` to land on user data `0x18` and user code `0x20` (with RPL 3: `0x1B` and `0x23`), the base in bits 63:48 must be `0x10`: `0x10 + 8 = 0x18` for `ss`, `0x10 + 16 = 0x20` for `cs`. (The CPU ORs in the RPL bits itself.)

So `STAR = (0x10 << 48) | (0x08 << 32)`.

## The entry stub

The calling convention for user code is the Linux one: the syscall **number** in `rax`, up to three arguments (we use three) in `rdi`, `rsi`, `rdx`, and the result comes back in `rax`. The kernel promises to preserve every other register except `rcx` and `r11` (the CPU overwrites those).

C expects its arguments in `rdi`, `rsi`, `rdx`, `rcx`, so the stub shuffles them. The order of operations is crucial: `rcx` holds the user's return address, so it has to be saved before it is used as an argument.

```
syscall_entry:
    mov  [user_rsp], rsp          ; remember the user stack (a single CPU, so one global will do)
    mov  rsp, [syscall_kernel_sp] ; a stack we trust
    push rcx                      ; user rip
    push r11                      ; user rflags
    push rdi, rsi, rdx, r8, r9, r10   ; C may clobber these; user code expects them back
    mov  rcx, rdx                 ; a3
    mov  rdx, rsi                 ; a2
    mov  rsi, rdi                 ; a1
    mov  rdi, rax                 ; the number
    call syscall_handler          ; result in rax
    pop  r10, r9, r8, rdx, rsi, rdi
    pop  r11
    pop  rcx
    mov  rsp, [user_rsp]
    sysretq
```

Rows with several registers stand for one `push` or `pop` per register. A real kernel keeps the stack pointers in a per-CPU area reached with `swapgs`; on one CPU a global is the simple version of the same idea. Callee-saved registers (`rbx`, `rbp`, `r12` to `r15`) need no work because the C function preserves them for you.

The stack stays 16-byte aligned at the `call` because eight 8-byte pushes follow an aligned `syscall_kernel_sp`.

### A famous trap

`sysretq` loads `rip` from `rcx`. If a user program could make `rcx` a non-canonical address, the CPU would fault *in ring 0 but with the user's stack*, which was a real vulnerability on some CPUs. Linux avoids it by returning with `iretq` whenever the return address is suspicious. Here we only return to code we started ourselves.

## Exiting

`SYS_EXIT` has nothing to return to. It does what your trap hook did in the last lesson: clear `in_user` and call `leave_user(code)`, so `run_user` returns the exit code.

## Your turn

The `rdmsr`/`wrmsr` helpers, the previous lesson's whole machinery (`run_user`, `user_init`, `leave_user`) and the globals `user_rsp` and `syscall_kernel_sp` are provided. Write `syscall_init`, `syscall_entry` and `syscall_handler`. The tests run ring 3 programs that do real `syscall` instructions.
