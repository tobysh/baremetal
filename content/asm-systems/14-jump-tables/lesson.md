---
title: Switch Statements and Jump Tables
summary: A dense switch becomes one bounds check and one indirect jump through a table of code addresses. Build a computed jump and a table of string pointers.
minutes: 40
difficulty: hard
tags: control-flow, jump-table, compiler
id: asm-systems/jump-tables
mode: harness
---
A chain of `if / else if` tests costs one comparison per case. When the cases are small consecutive integers, a compiler instead builds a **jump table**: an array of code addresses indexed by the value, so any case is reached with one bounds check and one indirect jump.

```c
long apply_op(long op, long a, long b) {
    switch (op) {
    case 0: return a + b;
    case 1: return a - b;
    case 2: return a * b;
    ...
    default: return -1;
    }
}
```

## The table

Put the table in read-only data. A label's address is a 64-bit constant, which `.quad` stores:

```asm
.section .rodata
.align 8
.Ltable:
    .quad .Ladd, .Lsub, .Lmul      # entry 0, 1, 2 ...

.text
```

Switch back to `.text` afterwards. The labels `.Ladd` etc. are ordinary code labels.

## The dispatch

```asm
cmp rdi, 7                  # 7 is the highest valid case
ja  .Ldefault               # UNSIGNED: also catches negative numbers
jmp qword ptr [.Ltable + rdi*8]
```

Two details matter. First, `ja` is an *unsigned* comparison, so a negative `op` (a huge unsigned number) is rejected by the same single check as `op > 7`. Second, `[.Ltable + rdi*8]` is an absolute address plus a scaled index. That form works because the test programs are linked as non-position-independent executables (`-no-pie`). Position-independent code first loads the table base with `lea` and then indexes: `lea rax, [rip + .Ltable]` / `jmp [rax + rdi*8]`. You can use either way here; the second works everywhere.

Every case ends in its own `ret`, because we jumped (not called) into it and the return address is still on top of the stack.

## Shifts need `cl`

A variable shift count must be in `cl`. Copy `b` from `rdx` into `rcx` first: `mov rcx, rdx` then `shl rax, cl`. The hardware masks the count to 6 bits (0-63) for 64-bit operands.

## Tables of data

The same trick works for data. A table of pointers to strings gives you `op_name`:

```asm
.section .rodata
.Lnames: .quad .Lsadd, .Lssub
.Lsadd:  .asciz "add"
```

`lea rax, [rip + .Lnames]` / `mov rax, [rax + rdi*8]` loads the pointer for index `rdi`.

## Why compilers do this

Jump tables turn a switch into constant time and are also the basis of virtual-method dispatch and bytecode interpreters (the final lesson). The cost is an indirect branch, which the CPU must predict; for a handful of cases a comparison chain can be faster, so compilers choose based on how dense and numerous the cases are.
