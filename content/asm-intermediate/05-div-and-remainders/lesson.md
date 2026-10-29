---
title: div, idiv and Remainders
summary: One division instruction gives both quotient and remainder in fixed registers. Learn rdx:rax, cqo, and how signed remainders differ from mathematical ones.
minutes: 30
difficulty: medium
tags: arithmetic, instructions, signed-unsigned
id: asm-intermediate/div-and-remainders
mode: harness
---
Division is the one arithmetic instruction with hard-wired registers. `div src` (unsigned) and `idiv src` (signed) divide the 128-bit value `rdx:rax` by `src`:

- the **quotient** goes to `rax`,
- the **remainder** goes to `rdx`.

So `n / d` and `n % d` come from the same instruction. If the quotient does not fit, or `d` is 0, the CPU raises a divide error (your program dies with SIGFPE).

## Unsigned: clear rdx

```asm
mov rax, rdi            # dividend
xor edx, edx            # high half = 0
div rsi                 # rax = rdi / rsi, rdx = rdi % rsi
```

Forgetting `xor edx, edx` divides a 128-bit number with garbage in the top half. This is the most common division bug.

## Signed: sign-extend with cqo

For `idiv`, the high half must be the sign extension of `rax`. `cqo` does exactly that (copy the sign of `rax` into every bit of `rdx`):

```asm
mov rax, rdi
cqo
idiv rsi
```

## Truncation versus floor

`idiv` truncates toward zero, like C. `-7 / 2` is `-3` and `-7 % 2` is `-1`: the remainder takes the sign of the dividend. Mathematics (and Python) prefer **floor** division, where `-7 // 2 = -4` and `-7 % 2 = 1`. To convert, adjust when the remainder is non-zero and the remainder and divisor have different signs: decrement the quotient, or add the divisor to the remainder.

`xor rdx, rsi` followed by `jns` is a compact "same sign?" test.

Writing through a pointer works as usual, but `rdx` is clobbered by `div`, so move a third argument out of `rdx` before dividing.
