---
title: Compare and Branch
summary: Use cmp and conditional jumps to implement if statements in assembly, and learn how signed and unsigned comparisons differ.
minutes: 20
difficulty: medium
id: asm-first-steps/clamp
mode: harness
---
Assembly has no `if`. It has **labels**, **jumps** and the flags from the last lesson. Everything else is built from those.

## cmp and test

`cmp a, b` subtracts `b` from `a`, throws the result away, and keeps only the flags. It is "compare" in the literal sense: afterwards the flags tell you how `a` relates to `b`.

`test a, b` does the same with a bitwise AND. `test rax, rax` is the usual way to ask "is `rax` zero or negative?".

## Conditional jumps

A **conditional jump** goes to a label if a condition holds, and otherwise falls through to the next instruction:

| Jump | Meaning after `cmp a, b` | Signedness |
|---|---|---|
| `je` / `jne` | `a == b` / `a != b` | either |
| `jl` / `jle` | `a < b` / `a <= b` | signed |
| `jg` / `jge` | `a > b` / `a >= b` | signed |
| `jb` / `jbe` | `a < b` / `a <= b` | unsigned ("below") |
| `ja` / `jae` | `a > b` / `a >= b` | unsigned ("above") |

Choosing between `jl` and `jb` is choosing between C's `long` and `unsigned long`. The CPU compares the same bits either way; only the jump you pick differs. `-1` is less than `1` as a signed number, but as an unsigned number it is the largest value of all.

## Labels

A label ending in a name is a global symbol. Labels starting with `.L` are **local**: GNU as keeps them private to the file, so you can reuse short names like `.Ldone` across functions.

```asm
    cmp rax, rsi
    jge .Lnot_low       # skip the next instruction if rax >= rsi
    mov rax, rsi
.Lnot_low:
```

## An if statement

The C code

```c
if (x < lo) x = lo;
```

becomes: compare, jump *over* the body when the condition is false, then the body.

```asm
    mov rax, rdi
    cmp rax, rsi        # flags from rax - rsi
    jge .Lskip          # if rax >= rsi, do not take the body
    mov rax, rsi        # body: x = lo
.Lskip:
```

Notice the jump uses the **opposite** condition of the C `if`: it jumps *around* the body.

## Common mistakes

- Reading `cmp a, b` backwards. `jl` is taken when `a < b`, with `a` being the *first* operand.
- Using an unsigned jump (`jb`) on signed data, so negative numbers misbehave.
- Changing the flags between `cmp` and the jump with another `add` or `sub`.
- Falling through into the wrong code. Each path should end at a `ret` or jump to a shared exit.

Now implement `clamp`, which squeezes a value into a range.
