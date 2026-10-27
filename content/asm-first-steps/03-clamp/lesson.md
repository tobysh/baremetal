---
title: Compare and Branch
summary: Use cmp and conditional jumps to implement if statements in assembly, and learn how signed and unsigned comparisons differ.
minutes: 20
difficulty: medium
tags: branching, flags
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

### Seeing the difference in bits

Take 8-bit numbers to keep it readable. The bit pattern `11111111` is `-1` if you read it as signed and `255` if you read it as unsigned. Now compare it with `00000001`:

| | signed reading | unsigned reading |
|---|---|---|
| `11111111` vs `00000001` | -1 < 1, so `jl` is taken | 255 > 1, so `jb` is **not** taken |

The CPU performs one subtraction and records several flags. The signed jumps look at `SF` and `OF` together (`jl` means "`SF` differs from `OF`"). The unsigned jumps look at the carry flag (`jb` means "`CF` is set": the subtraction had to borrow). You never need to remember that, but it explains why the choice is about *which flags* the jump reads, not about different data.

A good rule: **values** use the signedness of their C type (`long` is signed), while **addresses, sizes and counts** (`size_t`, pointers) are unsigned.

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

## Two ifs in a row, and shared exits

`clamp` needs two checks. The pattern is to apply them one after another, because after the first the value is already in `rax`:

```c
if (x < lo) x = lo;
if (x > hi) x = hi;
return x;
```

Each `if` becomes its own compare, opposite-condition jump and body, with its own local label. Because `lo <= hi`, a value moved up to `lo` can never then be above `hi`, so running both checks in order is correct.

You can also write it as separate paths that each return, like a `switch`:

```asm
    cmp rdi, rsi
    jl  .Lreturn_lo
    cmp rdi, rdx
    jg  .Lreturn_hi
    mov rax, rdi
    ret
.Lreturn_lo:
    mov rax, rsi
    ret
.Lreturn_hi:
    mov rax, rdx
    ret
```

Neither style is better. The first is shorter, the second makes each outcome easy to see. Whichever you pick, the thing to check is that **every path reaches a `ret` with `rax` set**.

## Tracing the flags

For `clamp(15, 0, 10)`, after `cmp rdi, rsi` (15 vs 0) the result of 15 - 0 is positive, so `jl` is not taken. After `cmp rdi, rdx` (15 vs 10) the result is positive, so `jg` is taken and you return `hi`. For `clamp(-3, 0, 10)` the first `cmp` computes -3 - 0, which is negative, so `jl` is taken. Run through each of the three cases (below, inside, above the range) plus the boundary values `x == lo` and `x == hi` before you submit: boundaries are where `jl` versus `jle` mistakes hide.

## Common mistakes

- Reading `cmp a, b` backwards. `jl` is taken when `a < b`, with `a` being the *first* operand.
- Using an unsigned jump (`jb`) on signed data, so negative numbers misbehave.
- Changing the flags between `cmp` and the jump with another `add` or `sub`.
- Falling through into the wrong code. Each path should end at a `ret` or jump to a shared exit.

Now implement `clamp`, which squeezes a value into a range.
