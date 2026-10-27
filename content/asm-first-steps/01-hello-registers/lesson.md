---
title: Hello, Registers
summary: Registers are the CPU's own tiny variables. Learn how a function receives its arguments and returns a result in x86-64 assembly.
minutes: 15
difficulty: easy
id: asm-first-steps/hello-registers
mode: harness
---
A C compiler turns your code into **machine code**: numbers the CPU executes directly. **Assembly** is that machine code written in readable form, one instruction per line. In this course you write it by hand.

## Registers

The CPU does its arithmetic in a few dozen tiny, very fast storage slots called **registers**. A 64-bit CPU has sixteen general-purpose ones: `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `rbp`, `rsp` and `r8` to `r15`. Each holds 64 bits (the size of a C `long`).

## Instructions

Every instruction has a name and operands. We use **Intel syntax**, where the *destination comes first*:

```asm
mov rax, 5        # rax = 5
add rax, rdx      # rax = rax + rdx
```

`mov dest, source` copies a value. `add dest, source` adds the source into the destination. Anything after `#` is a comment.

## How a function gets its arguments

When C calls a function, who puts the arguments where? Everyone agrees on a rulebook called the **System V AMD64 calling convention**. For whole numbers and pointers:

| Argument | Register |
|---|---|
| 1st | `rdi` |
| 2nd | `rsi` |
| 3rd | `rdx` |
| 4th | `rcx` |
| 5th | `r8` |
| 6th | `r9` |

The **return value** goes in `rax`. A function ends with `ret`, which jumps back to the caller.

So `long add_two(long a, long b)` receives `a` in `rdi` and `b` in `rsi`, and must leave `a + b` in `rax`.

## A complete function

```asm
.intel_syntax noprefix
.text
.globl double_it            # make the name visible to the C tests
double_it:                  # long double_it(long x)
    mov rax, rdi            # rax = x
    add rax, rdi            # rax = x + x
    ret

.section .note.GNU-stack,"",@progbits
```

The pieces:

- `.intel_syntax noprefix` selects the syntax with bare register names (keep it at the top).
- `.text` says "this is code". `.globl name` exports the function, and `name:` is its **label**, the address the caller jumps to.
- The last line is housekeeping that tells the system your code doesn't need an executable stack. Leave it at the end of every file.

## Common mistakes

- Swapping the operands: `mov rdi, rax` *writes* to `rdi`. The destination is on the left.
- Forgetting `ret`: the CPU keeps executing whatever bytes come next, and the program crashes.
- Returning the answer in the wrong register. The caller only looks at `rax`.

Now implement `add_two`.
