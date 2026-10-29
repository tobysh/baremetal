---
title: A Stack-Based Virtual Machine
summary: Write the heart of a bytecode interpreter in assembly: fetch an opcode, dispatch through a jump table, and use the machine stack as the VM's operand stack.
minutes: 60
difficulty: hard
tags: interpreter, jump-table, stack
id: asm-systems/stack-vm
mode: harness
---
Python, Java and WebAssembly all run on a **virtual machine**: a program that reads simple instructions ("bytecode") and carries them out. The core of every such interpreter is the same small loop, and it is usually written in C or assembly. You now have all the pieces to write one.

## The machine

Our VM is a **stack machine**. Programs are byte arrays; operands live on a stack of 64-bit integers. Each instruction is one opcode byte, sometimes followed by one operand byte.

| Opcode | Name | Effect |
|---|---|---|
| 0 | `HALT` | stop; the result is the value on top of the stack |
| 1 | `PUSH n` | push the next byte, sign-extended (-128..127) |
| 2 | `ADD` | pop `b`, pop `a`, push `a + b` |
| 3 | `SUB` | pop `b`, pop `a`, push `a - b` |
| 4 | `MUL` | pop `b`, pop `a`, push `a * b` |
| 5 | `DUP` | push a copy of the top value |
| 6 | `SWAP` | exchange the top two values |
| 7 | `POP` | discard the top value |
| 8 | `JNZ t` | pop a value; if non-zero, continue at byte offset `t` (the next byte, unsigned) |

Any other opcode ends the run immediately with the result `-1`. Programs given to you always have a value on the stack where one is needed.

For example `PUSH 3, PUSH 4, ADD, HALT` is the bytes `1 3 1 4 2 0` and returns 7.

## The interpreter loop

Registers: `rsi` = start of the code, `rcx` = program counter (an index), `rsp` = the VM stack. Using the real stack for operands makes the instructions tiny: `push`, `pop` and operations on `[rsp]`.

```asm
.Lnext:
    movzx eax, byte ptr [rsi + rcx]     # fetch opcode
    inc   rcx
    cmp   eax, 8
    ja    .Lbad
    jmp   qword ptr [table + rax*8]     # dispatch
```

(`movzx eax, ...` also clears the upper half of `rax`, so it is safe as a table index.) Each handler ends with `jmp .Lnext`. For example `ADD`:

```asm
.Ladd:
    pop rax
    add [rsp], rax          # [rsp] = a + b  (a was below b)
    jmp .Lnext
```

`SUB` must keep the order: with `b` on top, the result is `a - b`, so `sub [rsp], rax` after `pop rax` is right. `JNZ` is conditional assignment: read the target byte, `pop` the condition, then `cmovnz rcx, rdx` replaces the program counter only if it was non-zero (the target byte has to be consumed in either case).

## Cleaning up

The VM pushed values on your real stack, so before `ret` the stack pointer must be exactly what it was on entry, otherwise `ret` jumps to a garbage address. Save `rsp` at the start (`mov r8, rsp`) and restore it with `mov rsp, r8` on every exit path, `HALT` and the bad-opcode path.

## A real program

Doubling a number ten times, `2^10`: the stack holds `[acc, counter]`.

```text
 0: PUSH 1        acc = 1
 2: PUSH 10       counter = 10
 4: SWAP          loop: counter acc
 5: DUP
 6: ADD           acc = acc + acc
 7: SWAP
 8: PUSH 1
10: SUB           counter = counter - 1
11: DUP
12: JNZ 4         if counter != 0, goto 4  (consumes the duplicate)
14: POP           drop the counter
15: HALT          result: 1024
```

A dozen bytes of bytecode, and your assembly interpreter runs a loop. This is, in miniature, what every language implementation does.
