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

### Smaller views of the same register

A register is one 64-bit slot, but you can also name its low part. The instruction decides how many bits it touches, and the name you write picks the size:

| 64 bits | low 32 | low 16 | low 8 |
|---|---|---|---|
| `rax` | `eax` | `ax` | `al` |
| `rdi` | `edi` | `di` | `dil` |
| `r8` | `r8d` | `r8w` | `r8b` |

These are not separate registers. `eax` is the bottom half of `rax`, so `mov eax, 7` changes `rax`. One rule trips up almost everyone: **writing a 32-bit register zeroes the upper 32 bits** of the full register, but writing a 16-bit or 8-bit one leaves the rest alone. That is why you will see `xor eax, eax` rather than `xor rax, rax` to clear a register: it is shorter to encode and has the same effect.

For now stay with the 64-bit names, because a C `long` is 64 bits.

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

## What the caller sees

It helps to picture both sides of the call. When a C test program runs `long r = double_it(21);`, the compiler produces something like this:

```asm
mov rdi, 21          # put the first argument where the rulebook says
call double_it       # push the return address, jump to the label
# ...execution resumes here after your ret...
# the compiler now reads rax and stores it in r
```

Your function never "gets" arguments in any special way. They are simply values that happen to be sitting in registers when your first instruction runs, and the result is simply whatever is in `rax` when you `ret`. There is no type information at this level: the CPU does not know `rdi` is a `long`. The calling convention is a **contract** between the code that calls you and you, and the contract is only kept if both sides read the same rulebook.

## Tracing a function by hand

The best habit to build early is to trace code on paper, tracking each register after each line. For `double_it(21)`:

| After this line | `rdi` | `rax` |
|---|---|---|
| (entry) | 21 | unknown |
| `mov rax, rdi` | 21 | 21 |
| `add rax, rdi` | 21 | 42 |
| `ret` | 21 | 42, which the caller reads |

Notice that `rdi` was never changed: `add rax, rdi` only writes its *first* operand. Whenever you are unsure what a line does, ask "which operand gets written?" The answer is always the left one.

## From text to machine code

An assembler like GNU `as` turns each line into bytes. `mov rax, rdi` becomes the three bytes `48 89 F8`, and `ret` is the single byte `C3`. There is no magic in between: assembly is a one-to-one spelling of machine code. Lines that start with a dot (`.text`, `.globl`, `.intel_syntax`) are **directives**: they are instructions to the assembler about how to build the file, not instructions for the CPU, and they produce no code themselves.

## Common mistakes

- Swapping the operands: `mov rdi, rax` *writes* to `rdi`. The destination is on the left.
- Forgetting `ret`: the CPU keeps executing whatever bytes come next, and the program crashes.
- Returning the answer in the wrong register. The caller only looks at `rax`.

Now implement `add_two`.
