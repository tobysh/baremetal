---
title: Absolute Value and Flags
summary: Arithmetic instructions leave clues in the CPU's flags. Use neg and a conditional move to compute an absolute value without a branch.
minutes: 15
difficulty: easy
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

## Common mistakes

- Putting another arithmetic instruction between `neg` and `cmovs`. Every `add`, `sub`, `neg` and `cmp` overwrites the flags. `mov` does not touch them.
- Using an immediate with `cmov`: `cmovs rax, 5` is not allowed. Load the constant into a register first.
- Forgetting that the flags describe the *last* flag-setting instruction, not "the number in rax".

Now implement `abs_val`.
