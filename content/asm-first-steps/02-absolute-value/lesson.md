---
title: Absolute Value and Flags
summary: Arithmetic instructions leave clues in the CPU's flags. Use neg and a conditional move to compute an absolute value without a branch.
minutes: 15
difficulty: easy
tags: flags, arithmetic
id: asm-first-steps/absolute-value
mode: harness
---
In the last lesson you moved numbers between registers and added them. Real programs also need to *decide* things, and the CPU decides using **flags**.

## Flags

Almost every arithmetic instruction records facts about its result in a special register called `rflags`. The bits you will meet first:

| Flag | Name | Set when |
|---|---|---|
| `ZF` | zero | the result was 0 |
| `SF` | sign | the result was negative (top bit set) |
| `CF` | carry | an unsigned operation overflowed or borrowed |
| `OF` | overflow | a *signed* operation overflowed |

Nothing is stored in a variable: the flags are simply left behind for the next instruction to look at.

### Two's complement in one paragraph

Negative numbers are stored in **two's complement**: to negate a number, flip every bit and add one. Using 8 bits to keep it short, `5` is `00000101`. Flipping gives `11111010`, adding one gives `11111011`, and that is `-5`. The top bit is therefore the **sign bit**: it is 1 for every negative number. That is exactly what `SF` copies. The same hardware adds signed and unsigned numbers; only your interpretation changes, which is why there are separate flags (`CF` for unsigned, `OF` for signed) to tell you when each kind has gone out of range.

## Negating

`neg rax` replaces `rax` with `-rax` (two's complement) and sets the flags from the result. If the result is negative, `SF` is 1.

```asm
mov rax, rdi     # rax = x
neg rax          # rax = -x, and SF = 1 if -x < 0
```

## Conditional moves

A **conditional move** copies a value only if a flag condition holds, with no jump at all:

```asm
cmovs rax, rdi   # if SF is set: rax = rdi
```

Other forms follow the same pattern: `cmove` (equal / ZF set), `cmovne`, `cmovl` (signed less), `cmovg` (signed greater), `cmovns` (sign clear), and so on. The source can be a register or memory, but not an immediate number.

## Putting it together

The absolute value of `x` is `x` if it is not negative, otherwise `-x`. Compute `-x` first, then switch back to `x` if `-x` came out negative:

```asm
abs_val:
    mov rax, rdi     # rax = x
    neg rax          # rax = -x, flags describe -x
    cmovs rax, rdi   # if -x is negative, x was positive: rax = x
    ret
```

This works for `x = 0` too: `-0` is `0`, not negative, so `rax` stays `0`.

## Tracing it

Follow `abs_val(-7)` and `abs_val(7)` through the code, watching `rax` and `SF`:

| Step | x = -7: `rax` / `SF` | x = 7: `rax` / `SF` |
|---|---|---|
| `mov rax, rdi` | -7 / unchanged | 7 / unchanged |
| `neg rax` | 7 / 0 | -7 / 1 |
| `cmovs rax, rdi` | SF is 0, no move: **7** | SF is 1, move: rax = **7** |

Both inputs end with 7. The conditional move works "backwards" from how you might first think about it: the sign of `-x` is what is tested, and it tells you which of `x` and `-x` you want.

## Why avoid a jump?

You could write the same function with a compare and a jump, which is how Lesson 3 does things. A conditional move has one advantage: the CPU does not have to *guess* which way a branch will go. Modern processors run ahead of the instruction they are on, and a wrong guess about a jump throws that work away. A `cmov` has no guess to get wrong. It is not always faster (it always computes both values), but for tiny choices like this one it often is, and compilers use it constantly. If you compile `x < 0 ? -x : x` in C with optimisation on, you will usually see exactly the `neg` and `cmov` you wrote here.

## The one value with no twin

With 64 bits you can represent `-9223372036854775808` (`LONG_MIN`) but not `+9223372036854775808`. Negating `LONG_MIN` overflows and gives back `LONG_MIN`, which is still negative, so `SF` is 1 and `OF` is also set. Your function will return `LONG_MIN` for that input. In C this is **undefined behaviour** (`abs(INT_MIN)` is a classic trap), and the task tells you to ignore it. The lesson is that every fixed-width number system has an edge, and flags like `OF` exist so careful code can notice it.

## Common mistakes

- Putting another arithmetic instruction between `neg` and `cmovs`. Every `add`, `sub`, `neg` and `cmp` overwrites the flags. `mov` does not touch them.
- Using an immediate with `cmov`: `cmovs rax, 5` is not allowed. Load the constant into a register first.
- Forgetting that the flags describe the *last* flag-setting instruction, not "the number in rax".

Now implement `abs_val`.
